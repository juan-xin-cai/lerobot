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

from dataclasses import dataclass, field

from lerobot.cameras import CameraConfig

from ..config import RobotConfig

SUPPORTED_CONTROLLER_MODES = {"servo_control", "direct_control"}
SUPPORTED_GRIPPER_CONTROL_MODES = {"embedded_with_move_end_pose", "separate_servo"}
POSITION_KEYS = {"ee.x", "ee.y", "ee.z"}


@RobotConfig.register_subclass("airbot_play")
@dataclass(kw_only=True)
class AirbotPlayRobotConfig(RobotConfig):
    host: str = "localhost"
    port: int = 50051
    arm_dof: int = 6

    controller_mode: str = "servo_control"
    lease_ms: int = 15000
    renew_period_s: float = 5.0
    timeout_ms: int = 50
    control_rate_hz: float = 50.0

    set_arm_speed: float | None = None
    set_eef_speed: float | None = None
    default_eef_eff: float | None = None
    default_arm_eff: float | None = None
    blocking: bool = False

    cameras: dict[str, CameraConfig] = field(default_factory=dict)
    enable_gripper: bool = True
    gripper_control_mode: str = "embedded_with_move_end_pose"
    disconnect_return_zero: bool = False

    position_limits: dict[str, tuple[float, float]] | None = None
    max_position_step_m: float | dict[str, float] | None = 0.10
    gripper_limits: tuple[float, float] = (0.0, 1.0)

    def __post_init__(self) -> None:
        super().__post_init__()

        if self.arm_dof < 1:
            raise ValueError(f"`arm_dof` must be positive, got {self.arm_dof}.")
        if self.controller_mode not in SUPPORTED_CONTROLLER_MODES:
            raise ValueError(
                f"`controller_mode` must be one of {sorted(SUPPORTED_CONTROLLER_MODES)}, "
                f"got {self.controller_mode!r}."
            )
        if self.gripper_control_mode not in SUPPORTED_GRIPPER_CONTROL_MODES:
            raise ValueError(
                f"`gripper_control_mode` must be one of {sorted(SUPPORTED_GRIPPER_CONTROL_MODES)}, "
                f"got {self.gripper_control_mode!r}."
            )

        positive_values = {
            "lease_ms": self.lease_ms,
            "renew_period_s": self.renew_period_s,
            "timeout_ms": self.timeout_ms,
            "control_rate_hz": self.control_rate_hz,
        }
        for name, value in positive_values.items():
            if value <= 0:
                raise ValueError(f"`{name}` must be positive, got {value}.")

        if self.position_limits is not None:
            invalid_keys = set(self.position_limits) - POSITION_KEYS
            if invalid_keys:
                raise ValueError(
                    f"`position_limits` keys must be a subset of {sorted(POSITION_KEYS)}, "
                    f"got {sorted(invalid_keys)}."
                )
            for key, (low, high) in self.position_limits.items():
                if low > high:
                    raise ValueError(f"`position_limits[{key}]` must satisfy low <= high, got {(low, high)}.")

        if isinstance(self.max_position_step_m, dict):
            invalid_keys = set(self.max_position_step_m) - POSITION_KEYS
            if invalid_keys:
                raise ValueError(
                    f"`max_position_step_m` keys must be a subset of {sorted(POSITION_KEYS)}, "
                    f"got {sorted(invalid_keys)}."
                )
            for key, value in self.max_position_step_m.items():
                if value < 0:
                    raise ValueError(f"`max_position_step_m[{key}]` must be non-negative, got {value}.")
        elif self.max_position_step_m is not None and self.max_position_step_m < 0:
            raise ValueError(f"`max_position_step_m` must be non-negative, got {self.max_position_step_m}.")

        gripper_low, gripper_high = self.gripper_limits
        if gripper_low > gripper_high:
            raise ValueError(
                f"`gripper_limits` must satisfy low <= high, got {self.gripper_limits}."
            )
