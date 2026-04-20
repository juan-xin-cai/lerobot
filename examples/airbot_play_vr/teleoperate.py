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

from lerobot.configs import parser
from lerobot.robots.airbot_play import AirbotPlayRobotConfig
from lerobot.teleoperators.airbot_vr import AirbotVRTeleoperatorConfig
from lerobot.utils.robot_utils import precise_sleep
from lerobot.utils.utils import init_logging
from lerobot.utils.visualization_utils import init_rerun, log_rerun_data, shutdown_rerun

from lerobot.robots.airbot_play.airbot_play import AirbotPlayRobot
from lerobot.teleoperators.airbot_vr.airbot_vr import AirbotVRTeleoperator


@dataclass
class AirbotPlayVRTeleoperateConfig:
    robot: AirbotPlayRobotConfig = field(default_factory=AirbotPlayRobotConfig)
    teleop: AirbotVRTeleoperatorConfig = field(default_factory=AirbotVRTeleoperatorConfig)
    fps: int = 30
    teleop_time_s: float | None = None
    display_data: bool = False


@parser.wrap()
def teleoperate(cfg: AirbotPlayVRTeleoperateConfig) -> None:
    init_logging()
    logging.info(pformat(asdict(cfg)))

    robot = AirbotPlayRobot(cfg.robot)
    teleop = AirbotVRTeleoperator(cfg.teleop)

    start_time = time.perf_counter()
    rerun_started = False
    try:
        if cfg.display_data:
            init_rerun(session_name="airbot_play_vr_teleoperate")
            rerun_started = True

        robot.connect()
        teleop.connect()

        while True:
            loop_start = time.perf_counter()
            observation = robot.get_observation()
            teleop_events = teleop.get_teleop_events()

            if teleop_events.get("reset_robot", False):
                logging.info("Received reset_robot event from VR, resetting Airbot to home.")
                robot.reset_to_home()
                precise_sleep(0.5)
                continue

            if teleop_events.get("recenter_vr", False):
                logging.info("Received recenter_vr event from VR.")

            action = teleop.get_action()
            sent_action = robot.send_action(action)

            if cfg.display_data:
                log_rerun_data(observation=observation, action=sent_action)

            precise_sleep(max(1.0 / cfg.fps - (time.perf_counter() - loop_start), 0.0))
            if cfg.teleop_time_s is not None and time.perf_counter() - start_time >= cfg.teleop_time_s:
                break
    finally:
        if teleop.is_connected:
            teleop.disconnect()
        if robot.is_connected:
            robot.disconnect()
        if rerun_started:
            shutdown_rerun()


def main() -> None:
    teleoperate()


if __name__ == "__main__":
    main()
