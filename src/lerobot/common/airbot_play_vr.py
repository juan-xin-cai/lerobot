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

from typing import Any

import numpy as np

from lerobot.utils.rotation import Rotation

AIRBOT_ACTION_FEATURE_NAMES: tuple[str, ...] = (
    "ee.x",
    "ee.y",
    "ee.z",
    "ee.wx",
    "ee.wy",
    "ee.wz",
    "ee.gripper_pos",
)

AIRBOT_VR_AUX_NAMES: tuple[str, ...] = (
    "teleop.vr.pos.x",
    "teleop.vr.pos.y",
    "teleop.vr.pos.z",
    "teleop.vr.quat.w",
    "teleop.vr.quat.x",
    "teleop.vr.quat.y",
    "teleop.vr.quat.z",
    "teleop.vr.trigger",
    "teleop.vr.grip",
    "teleop.vr.tracking_ok",
)

AIRBOT_PACKET_EVENT_NAMES: tuple[str, ...] = (
    "reset_robot",
    "recenter_vr",
    "rerecord_episode",
    "terminate_episode",
)

AIRBOT_PACKET_BUTTON_NAMES: tuple[str, ...] = (
    "trigger",
    "grip",
    "a",
    "b",
    "menu",
    "joystick_press",
)

AIRBOT_PACKET_ANALOG_NAMES: tuple[str, ...] = (
    "trigger",
    "grip",
    "joystick_x",
    "joystick_y",
)

AIRBOT_FRAME_NAME = "airbot_base"


def make_airbot_action_features() -> dict[str, type]:
    return dict.fromkeys(AIRBOT_ACTION_FEATURE_NAMES, float)


def make_airbot_vr_aux_dataset_features() -> dict[str, dict[str, Any]]:
    return {
        "observation.teleop_vr": {
            "dtype": "float32",
            "shape": (len(AIRBOT_VR_AUX_NAMES),),
            "names": list(AIRBOT_VR_AUX_NAMES),
        }
    }


def quat_wxyz_to_rotvec(quat_wxyz: list[float] | tuple[float, ...] | np.ndarray) -> np.ndarray:
    quat_wxyz_arr = np.asarray(quat_wxyz, dtype=float)
    if quat_wxyz_arr.shape != (4,):
        raise ValueError(f"Expected quaternion in wxyz format with shape (4,), got {quat_wxyz_arr.shape}")

    quat_xyzw = np.array(
        [quat_wxyz_arr[1], quat_wxyz_arr[2], quat_wxyz_arr[3], quat_wxyz_arr[0]],
        dtype=float,
    )
    return Rotation.from_quat(quat_xyzw).as_rotvec()


def rotvec_to_quat_xyzw(rotvec: list[float] | tuple[float, ...] | np.ndarray) -> np.ndarray:
    rotvec_arr = np.asarray(rotvec, dtype=float)
    if rotvec_arr.shape != (3,):
        raise ValueError(f"Expected rotation vector with shape (3,), got {rotvec_arr.shape}")

    return Rotation.from_rotvec(rotvec_arr).as_quat()


def flatten_vr_auxiliary_data(packet: dict[str, Any] | None) -> dict[str, float]:
    packet = packet or {}
    pose = packet.get("pose") or {}
    analog = packet.get("analog") or {}

    pos = np.asarray(pose.get("pos", [0.0, 0.0, 0.0]), dtype=float)
    quat_wxyz = np.asarray(pose.get("quat_wxyz", [1.0, 0.0, 0.0, 0.0]), dtype=float)

    if pos.shape != (3,):
        pos = np.array([0.0, 0.0, 0.0], dtype=float)
    if quat_wxyz.shape != (4,):
        quat_wxyz = np.array([1.0, 0.0, 0.0, 0.0], dtype=float)

    return {
        "teleop.vr.pos.x": float(pos[0]),
        "teleop.vr.pos.y": float(pos[1]),
        "teleop.vr.pos.z": float(pos[2]),
        "teleop.vr.quat.w": float(quat_wxyz[0]),
        "teleop.vr.quat.x": float(quat_wxyz[1]),
        "teleop.vr.quat.y": float(quat_wxyz[2]),
        "teleop.vr.quat.z": float(quat_wxyz[3]),
        "teleop.vr.trigger": float(analog.get("trigger", 0.0)),
        "teleop.vr.grip": float(analog.get("grip", 0.0)),
        "teleop.vr.tracking_ok": float(bool(packet.get("tracking_ok", False))),
    }
