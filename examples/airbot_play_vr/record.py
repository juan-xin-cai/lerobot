#!/usr/bin/env python

# Copyright 2026 The HuggingFace Inc. team. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import annotations

import logging
import time
from dataclasses import asdict, dataclass, field
from pprint import pformat

import numpy as np

from lerobot.common.airbot_play_vr import AIRBOT_VR_AUX_NAMES, make_airbot_vr_aux_dataset_features
from lerobot.common.control_utils import (
    init_keyboard_listener,
    is_headless,
    sanity_check_dataset_name,
    sanity_check_dataset_robot_compatibility,
)
from lerobot.configs import parser
from lerobot.datasets import (
    LeRobotDataset,
    VideoEncodingManager,
)
from lerobot.robots.airbot_play import AirbotPlayRobotConfig
from lerobot.scripts.lerobot_record import DatasetRecordConfig
from lerobot.teleoperators.airbot_vr import AirbotVRTeleoperatorConfig
from lerobot.teleoperators.utils import TeleopEvents
from lerobot.utils.constants import ACTION, OBS_IMAGES, OBS_STATE
from lerobot.utils.feature_utils import build_dataset_frame, combine_feature_dicts, hw_to_dataset_features
from lerobot.utils.robot_utils import precise_sleep
from lerobot.utils.utils import init_logging, log_say
from lerobot.utils.visualization_utils import init_rerun, log_rerun_data, shutdown_rerun

from lerobot.robots.airbot_play.airbot_play import AirbotPlayRobot
from lerobot.teleoperators.airbot_vr.airbot_vr import AirbotVRTeleoperator


def _default_dataset_config() -> DatasetRecordConfig:
    return DatasetRecordConfig(
        repo_id="your-hf-user/airbot-play-vr",
        single_task="Teleoperate the Airbot Play arm with a VR controller.",
    )


@dataclass
class AirbotPlayVRRecordConfig:
    robot: AirbotPlayRobotConfig = field(default_factory=AirbotPlayRobotConfig)
    teleop: AirbotVRTeleoperatorConfig = field(default_factory=AirbotVRTeleoperatorConfig)
    dataset: DatasetRecordConfig = field(default_factory=_default_dataset_config)
    display_data: bool = False
    play_sounds: bool = True
    resume: bool = False


def _make_observation_dataset_features(robot: AirbotPlayRobot, use_videos: bool) -> dict[str, dict]:
    state_names: list[str] = []
    image_features: dict[str, tuple[int, int, int]] = {}

    for name, feature in robot.observation_features.items():
        if isinstance(feature, tuple):
            dataset_key = name if name.startswith(f"{OBS_IMAGES}.") else f"{OBS_IMAGES}.{name}"
            image_features[dataset_key] = feature
        else:
            state_names.append(name)

    features: dict[str, dict] = {}
    if state_names:
        features[OBS_STATE] = {
            "dtype": "float32",
            "shape": (len(state_names),),
            "names": state_names,
        }
    for dataset_key, shape in image_features.items():
        features[dataset_key] = {
            "dtype": "video" if use_videos else "image",
            "shape": shape,
            "names": ["height", "width", "channels"],
        }
    return features


def _build_dataset(
    cfg: AirbotPlayVRRecordConfig,
    robot: AirbotPlayRobot,
) -> LeRobotDataset:
    dataset_features = combine_feature_dicts(
        hw_to_dataset_features(robot.action_features, ACTION, use_video=cfg.dataset.video),
        _make_observation_dataset_features(robot, use_videos=cfg.dataset.video),
        make_airbot_vr_aux_dataset_features(),
    )

    if cfg.resume:
        dataset = LeRobotDataset.resume(
            cfg.dataset.repo_id,
            root=cfg.dataset.root,
            batch_encoding_size=cfg.dataset.video_encoding_batch_size,
            vcodec=cfg.dataset.vcodec,
            streaming_encoding=cfg.dataset.streaming_encoding,
            encoder_queue_maxsize=cfg.dataset.encoder_queue_maxsize,
            encoder_threads=cfg.dataset.encoder_threads,
            image_writer_processes=cfg.dataset.num_image_writer_processes,
            image_writer_threads=cfg.dataset.num_image_writer_threads_per_camera * len(robot.cameras),
        )
        sanity_check_dataset_robot_compatibility(dataset, robot, cfg.dataset.fps, dataset_features)
        return dataset

    sanity_check_dataset_name(cfg.dataset.repo_id, policy_cfg=None)
    return LeRobotDataset.create(
        repo_id=cfg.dataset.repo_id,
        fps=cfg.dataset.fps,
        root=cfg.dataset.root,
        robot_type=robot.name,
        features=dataset_features,
        use_videos=cfg.dataset.video,
        image_writer_processes=cfg.dataset.num_image_writer_processes,
        image_writer_threads=cfg.dataset.num_image_writer_threads_per_camera * len(robot.cameras),
        batch_encoding_size=cfg.dataset.video_encoding_batch_size,
        vcodec=cfg.dataset.vcodec,
        streaming_encoding=cfg.dataset.streaming_encoding,
        encoder_queue_maxsize=cfg.dataset.encoder_queue_maxsize,
        encoder_threads=cfg.dataset.encoder_threads,
    )


def _add_dataset_frame(
    dataset: LeRobotDataset,
    observation: dict,
    action: dict,
    teleop: AirbotVRTeleoperator,
    task: str,
) -> None:
    observation_frame: dict[str, np.ndarray] = {}
    state_feature = dataset.features.get(OBS_STATE)
    if state_feature is not None:
        observation_frame[OBS_STATE] = np.array(
            [observation[name] for name in state_feature["names"]],
            dtype=np.float32,
        )
    for feature_name, feature in dataset.features.items():
        if feature_name.startswith(f"{OBS_IMAGES}.") and feature["dtype"] in ("image", "video"):
            camera_name = feature_name.removeprefix(f"{OBS_IMAGES}.")
            if feature_name in observation:
                observation_frame[feature_name] = observation[feature_name]
            else:
                observation_frame[feature_name] = observation[camera_name]

    auxiliary_data = (
        teleop.latest_vr_auxiliary_data
        if hasattr(teleop, "latest_vr_auxiliary_data")
        else teleop.get_vr_auxiliary_data()
    )
    observation_frame["observation.teleop_vr"] = np.array(
        [float(auxiliary_data[name]) for name in AIRBOT_VR_AUX_NAMES],
        dtype=np.float32,
    )
    action_frame = build_dataset_frame(dataset.features, action, prefix=ACTION)
    dataset.add_frame({**observation_frame, **action_frame, "task": task})


def _control_loop(
    robot: AirbotPlayRobot,
    teleop: AirbotVRTeleoperator,
    dataset: LeRobotDataset | None,
    control_time_s: float,
    fps: int,
    task: str,
    loop_events: dict[str, bool],
    display_data: bool,
) -> None:
    start_time = time.perf_counter()
    while time.perf_counter() - start_time < control_time_s:
        if loop_events["exit_early"]:
            loop_events["exit_early"] = False
            return

        loop_start = time.perf_counter()
        observation = robot.get_observation()
        teleop_events = teleop.get_teleop_events()

        if teleop_events.get("reset_robot", False):
            logging.info("Received reset_robot during recording, pausing control loop.")
            robot.reset_to_home()
            precise_sleep(0.5)
            continue

        if teleop_events.get(TeleopEvents.TERMINATE_EPISODE, False):
            loop_events["exit_early"] = True
            return
        if teleop_events.get(TeleopEvents.RERECORD_EPISODE, False):
            loop_events["rerecord_episode"] = True
            loop_events["exit_early"] = True
            return

        action = teleop.get_action()
        sent_action = robot.send_action(action)

        if dataset is not None:
            _add_dataset_frame(dataset, observation, sent_action, teleop, task)

        if display_data:
            log_rerun_data(observation=observation, action=sent_action)

        precise_sleep(max(1.0 / fps - (time.perf_counter() - loop_start), 0.0))


@parser.wrap()
def record(cfg: AirbotPlayVRRecordConfig) -> LeRobotDataset:
    init_logging()
    logging.info(pformat(asdict(cfg)))

    robot = AirbotPlayRobot(cfg.robot)
    teleop = AirbotVRTeleoperator(cfg.teleop)

    dataset: LeRobotDataset | None = None
    listener = None
    rerun_started = False

    try:
        if cfg.display_data:
            init_rerun(session_name="airbot_play_vr_record")
            rerun_started = True

        robot.connect()
        teleop.connect()

        dataset = _build_dataset(cfg, robot)
        listener, loop_events = init_keyboard_listener()

        with VideoEncodingManager(dataset):
            recorded_episodes = 0
            while recorded_episodes < cfg.dataset.num_episodes and not loop_events["stop_recording"]:
                log_say(f"Recording episode {recorded_episodes + 1}", cfg.play_sounds)
                _control_loop(
                    robot=robot,
                    teleop=teleop,
                    dataset=dataset,
                    control_time_s=cfg.dataset.episode_time_s,
                    fps=cfg.dataset.fps,
                    task=cfg.dataset.single_task,
                    loop_events=loop_events,
                    display_data=cfg.display_data,
                )

                if loop_events["rerecord_episode"]:
                    dataset.clear_episode_buffer()
                    loop_events["rerecord_episode"] = False
                    loop_events["exit_early"] = False
                    continue

                dataset.save_episode()
                recorded_episodes += 1

                if recorded_episodes >= cfg.dataset.num_episodes or loop_events["stop_recording"]:
                    break

                log_say("Reset the environment", cfg.play_sounds)
                _control_loop(
                    robot=robot,
                    teleop=teleop,
                    dataset=None,
                    control_time_s=cfg.dataset.reset_time_s,
                    fps=cfg.dataset.fps,
                    task=cfg.dataset.single_task,
                    loop_events=loop_events,
                    display_data=cfg.display_data,
                )
    finally:
        log_say("Stop recording", cfg.play_sounds, blocking=True)

        try:
            if dataset is not None:
                dataset.finalize()
        finally:
            if not is_headless() and listener is not None:
                listener.stop()
            if teleop.is_connected:
                teleop.disconnect()
            if robot.is_connected:
                robot.disconnect()
            if rerun_started:
                shutdown_rerun()
        if dataset is not None and cfg.dataset.push_to_hub:
            dataset.push_to_hub(tags=cfg.dataset.tags, private=cfg.dataset.private)

    if dataset is None:
        raise RuntimeError("Airbot Play VR dataset was not initialized.")
    return dataset


def main() -> None:
    record()


if __name__ == "__main__":
    main()
