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

from types import SimpleNamespace
from unittest.mock import MagicMock, patch

import numpy as np
import pytest

from lerobot.cameras.opencv import OpenCVCameraConfig
from lerobot.robots.airbot_play import AirbotPlayRobot, AirbotPlayRobotConfig


class FakeCartesianPose:
    def __init__(self, position, orientation):
        self.position = tuple(position)
        self.orientation = tuple(orientation)


class FakeArmControlOptions:
    pass


def _make_client_mock() -> MagicMock:
    client = MagicMock(name="AirbotClientMock")
    client.acquire_control.return_value = True
    client.switch_mode.return_value = True
    client.set_arm_speed.return_value = True
    client.set_eef_speed.return_value = True
    client.release_control.return_value = True
    client.close.return_value = None
    client.get_end_pose.return_value = SimpleNamespace(position=(0.40, 0.10, 0.20), orientation=(0.0, 0.0, 0.0, 1.0))
    client.get_joint_pos.return_value = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6]
    client.get_joint_vel.return_value = [0.01, 0.02, 0.03, 0.04, 0.05, 0.06]
    client.get_eef_pos.return_value = 0.55
    client.move_end_pose.return_value = True
    client.servo_eef_pos.return_value = True
    client.return_zero.return_value = True
    return client


def _make_camera_mock(height: int = 240, width: int = 320) -> MagicMock:
    camera = MagicMock(name="AirbotCameraMock")
    camera.connect = MagicMock()
    camera.disconnect = MagicMock()
    camera.read_latest = MagicMock(return_value=np.zeros((height, width, 3), dtype=np.uint8))
    camera.is_connected = True
    return camera


@pytest.fixture
def airbot_robot():
    client = _make_client_mock()
    camera = _make_camera_mock()
    cameras = {"wrist": camera}

    with (
        patch("lerobot.robots.airbot_play.airbot_play.require_package", return_value=None),
        patch("lerobot.robots.airbot_play.airbot_play.AirbotClient", side_effect=lambda *args, **kwargs: client),
        patch("lerobot.robots.airbot_play.airbot_play.CartesianPose", FakeCartesianPose),
        patch("lerobot.robots.airbot_play.airbot_play.ArmControlOptions", FakeArmControlOptions),
        patch("lerobot.robots.airbot_play.airbot_play.make_cameras_from_configs", return_value=cameras),
    ):
        config = AirbotPlayRobotConfig(
            host="127.0.0.1",
            port=50051,
            set_arm_speed=0.35,
            set_eef_speed=0.15,
            cameras={
                "wrist": OpenCVCameraConfig(index_or_path=0, fps=30, width=320, height=240),
            },
        )
        robot = AirbotPlayRobot(config)
        yield robot, client, camera
        if robot.is_connected:
            robot.disconnect()


def _extract_call_arg(call, key: str, fallback_index: int):
    if key in call.kwargs:
        return call.kwargs[key]
    return call.args[fallback_index]


def test_config_defaults_and_registration():
    config = AirbotPlayRobotConfig()
    assert config.type == "airbot_play"
    assert config.controller_mode == "servo_control"
    assert config.gripper_control_mode == "embedded_with_move_end_pose"


def test_connect_disconnect(airbot_robot):
    robot, client, camera = airbot_robot

    assert not robot.is_connected

    robot.connect()
    assert robot.is_connected
    client.acquire_control.assert_called_once_with(lease_ms=15000, renew_period_s=5.0)
    client.switch_mode.assert_called_once_with("servo_control")
    client.set_arm_speed.assert_called_once_with(0.35)
    client.set_eef_speed.assert_called_once_with(0.15)
    camera.connect.assert_called_once()

    robot.disconnect()
    assert not robot.is_connected
    client.release_control.assert_called_once()
    client.close.assert_called_once()
    camera.disconnect.assert_called_once()


def test_connect_cleans_up_when_camera_connection_fails():
    client = _make_client_mock()
    camera = _make_camera_mock()
    camera.connect.side_effect = RuntimeError("camera boom")

    with (
        patch("lerobot.robots.airbot_play.airbot_play.require_package", return_value=None),
        patch("lerobot.robots.airbot_play.airbot_play.AirbotClient", side_effect=lambda *args, **kwargs: client),
        patch("lerobot.robots.airbot_play.airbot_play.CartesianPose", FakeCartesianPose),
        patch("lerobot.robots.airbot_play.airbot_play.ArmControlOptions", FakeArmControlOptions),
        patch(
            "lerobot.robots.airbot_play.airbot_play.make_cameras_from_configs",
            return_value={"wrist": camera},
        ),
    ):
        robot = AirbotPlayRobot(
            AirbotPlayRobotConfig(
                cameras={"wrist": OpenCVCameraConfig(index_or_path=0, fps=30, width=320, height=240)}
            )
        )
        with pytest.raises(RuntimeError, match="camera boom"):
            robot.connect()

    client.release_control.assert_called_once()
    client.close.assert_called_once()


def test_get_observation_returns_plan_fields(airbot_robot):
    robot, _, _ = airbot_robot
    robot.connect()

    observation = robot.get_observation()

    expected_keys = {
        "ee.x",
        "ee.y",
        "ee.z",
        "ee.wx",
        "ee.wy",
        "ee.wz",
        "ee.gripper_pos",
        *(f"joint_{idx}.pos" for idx in range(1, 7)),
        *(f"joint_{idx}.vel" for idx in range(1, 7)),
        "observation.images.wrist",
    }
    assert set(observation) == expected_keys
    assert observation["ee.x"] == pytest.approx(0.40)
    assert observation["ee.wz"] == pytest.approx(0.0)
    assert observation["ee.gripper_pos"] == pytest.approx(0.55)
    assert observation["joint_6.pos"] == pytest.approx(0.6)
    assert observation["joint_6.vel"] == pytest.approx(0.06)
    assert observation["observation.images.wrist"].shape == (240, 320, 3)


def test_send_action_converts_rotvec_to_quaternion_and_embeds_gripper(airbot_robot):
    robot, client, _ = airbot_robot
    robot.connect()

    sent = robot.send_action(
        {
            "ee.x": 0.45,
            "ee.y": 0.08,
            "ee.z": 0.18,
            "ee.wx": 0.0,
            "ee.wy": 0.0,
            "ee.wz": np.pi,
            "ee.gripper_pos": 0.8,
        }
    )

    move_call = client.move_end_pose.call_args
    pose = _extract_call_arg(move_call, "pos", 0)
    options = _extract_call_arg(move_call, "options", 1)

    np.testing.assert_allclose(pose.position, [0.45, 0.08, 0.18], atol=1e-6)
    np.testing.assert_allclose(pose.orientation, [0.0, 0.0, 1.0, 0.0], atol=1e-6)
    assert options.eef_pos == pytest.approx(0.8)
    assert sent["ee.gripper_pos"] == pytest.approx(0.8)


def test_send_action_falls_back_to_separate_gripper_servo_when_embedded_move_fails(airbot_robot):
    robot, client, _ = airbot_robot
    robot.connect()
    client.move_end_pose.side_effect = [RuntimeError("embedded failed"), True]

    robot.send_action(
        {
            "ee.x": 0.42,
            "ee.y": 0.11,
            "ee.z": 0.19,
            "ee.wx": 0.0,
            "ee.wy": 0.0,
            "ee.wz": 0.0,
            "ee.gripper_pos": 0.25,
        }
    )

    assert client.move_end_pose.call_count == 2
    first_options = _extract_call_arg(client.move_end_pose.call_args_list[0], "options", 1)
    second_options = _extract_call_arg(client.move_end_pose.call_args_list[1], "options", 1)
    assert first_options.eef_pos == pytest.approx(0.25)
    assert not hasattr(second_options, "eef_pos")
    client.servo_eef_pos.assert_called_once()
    assert robot._gripper_control_mode == "separate_servo"


def test_send_action_rejects_missing_fields(airbot_robot):
    robot, _, _ = airbot_robot
    robot.connect()

    with pytest.raises(KeyError, match="missing keys"):
        robot.send_action({"ee.x": 0.4})


def test_reset_to_home_calls_return_zero(airbot_robot):
    robot, client, _ = airbot_robot
    robot.connect()

    robot.reset_to_home()
    client.return_zero.assert_called_once()


def test_missing_arm_sdk_raises_clear_error():
    with pytest.raises(ImportError, match="vendor-provided `arm_sdk` wheel"):
        AirbotPlayRobot(AirbotPlayRobotConfig())
