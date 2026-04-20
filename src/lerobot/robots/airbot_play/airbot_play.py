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
from functools import cached_property
from typing import TYPE_CHECKING, Any

import numpy as np

from lerobot.cameras import make_cameras_from_configs
from lerobot.types import RobotAction, RobotObservation
from lerobot.utils.decorators import check_if_already_connected, check_if_not_connected
from lerobot.utils.import_utils import _arm_sdk_available, require_package

from ..robot import Robot
from .config_airbot_play import AirbotPlayRobotConfig

if TYPE_CHECKING or _arm_sdk_available:
    from arm_sdk import AirbotClient, ArmControlOptions, CartesianPose
else:
    AirbotClient = None
    ArmControlOptions = None
    CartesianPose = None

logger = logging.getLogger(__name__)

EE_ACTION_KEYS = (
    "ee.x",
    "ee.y",
    "ee.z",
    "ee.wx",
    "ee.wy",
    "ee.wz",
    "ee.gripper_pos",
)
POSITION_KEYS = EE_ACTION_KEYS[:3]
ROTATION_KEYS = EE_ACTION_KEYS[3:6]
_MISSING = object()


def _rotvec_to_quat_xyzw(rotvec: np.ndarray) -> np.ndarray:
    angle = float(np.linalg.norm(rotvec))
    if angle < 1e-12:
        return np.array([0.0, 0.0, 0.0, 1.0], dtype=float)

    axis = rotvec / angle
    sin_half = np.sin(angle / 2.0)
    return np.concatenate((axis * sin_half, np.array([np.cos(angle / 2.0)], dtype=float)))


def _quat_xyzw_to_rotvec(quat_xyzw: np.ndarray) -> np.ndarray:
    quat = np.asarray(quat_xyzw, dtype=float).reshape(4)
    norm = float(np.linalg.norm(quat))
    if norm < 1e-12:
        return np.zeros(3, dtype=float)

    quat = quat / norm
    if quat[3] < 0:
        quat = -quat

    xyz = quat[:3]
    xyz_norm = float(np.linalg.norm(xyz))
    if xyz_norm < 1e-12:
        return np.zeros(3, dtype=float)

    angle = 2.0 * np.arctan2(xyz_norm, quat[3])
    return xyz / xyz_norm * angle


class AirbotPlayRobot(Robot):
    config_class = AirbotPlayRobotConfig
    name = "airbot_play"

    def __init__(self, config: AirbotPlayRobotConfig):
        require_package(
            "arm_sdk",
            extra="airbot_play",
            allow_module_without_metadata=True,
            install_hint=(
                "Install the vendor-provided `arm_sdk` wheel in this Python environment, "
                "then retry. If you installed LeRobot from source, you can also install "
                "`lerobot[airbot_play]` for the LeRobot-side feature entry."
            ),
        )
        super().__init__(config)

        self.config = config
        self._client: Any | None = None
        self._is_connected = False
        self._lease_acquired = False
        self._gripper_control_mode = config.gripper_control_mode

        self.cameras = make_cameras_from_configs(config.cameras)

        self._last_pose: tuple[np.ndarray, np.ndarray] | None = None
        self._last_joint_positions: np.ndarray | None = None
        self._last_joint_velocities: np.ndarray | None = None
        self._last_gripper_pos: float | None = None
        self._last_action: RobotAction | None = None

    @cached_property
    def observation_features(self) -> dict[str, type | tuple[int | None, int | None, int]]:
        features: dict[str, type | tuple[int | None, int | None, int]] = dict.fromkeys(EE_ACTION_KEYS, float)
        for joint_idx in range(1, self.config.arm_dof + 1):
            features[f"joint_{joint_idx}.pos"] = float
            features[f"joint_{joint_idx}.vel"] = float
        for name, camera_config in self.config.cameras.items():
            features[f"observation.images.{name}"] = (camera_config.height, camera_config.width, 3)
        return features

    @cached_property
    def action_features(self) -> dict[str, type]:
        return dict.fromkeys(EE_ACTION_KEYS, float)

    @property
    def is_connected(self) -> bool:
        return self._is_connected

    @property
    def is_calibrated(self) -> bool:
        return True

    @check_if_already_connected
    def connect(self, calibrate: bool = True) -> None:
        del calibrate

        client = self._make_client()
        self._client = client
        connected_cameras: list[str] = []
        try:
            if not self._acquire_control():
                raise ConnectionError("Failed to acquire Airbot Play control lease.")
            self._lease_acquired = True

            self.configure()

            for name, camera in self.cameras.items():
                camera.connect()
                connected_cameras.append(name)

            self._prime_state_cache()
            self._is_connected = True
            logger.info("%s connected.", self)
        except Exception:
            for name in reversed(connected_cameras):
                try:
                    self.cameras[name].disconnect()
                except Exception:  # nosec B110
                    logger.warning("Failed to disconnect Airbot camera '%s' during cleanup.", name, exc_info=True)
            self._best_effort_release_client()
            raise

    def calibrate(self) -> None:
        return None

    def configure(self) -> None:
        self._set_controller_mode()
        self._set_speed_if_requested(("set_arm_speed",), self.config.set_arm_speed)
        self._set_speed_if_requested(("set_eef_speed",), self.config.set_eef_speed)

    @check_if_not_connected
    def get_observation(self) -> RobotObservation:
        observation: RobotObservation = {}

        try:
            position, quaternion = self._read_end_pose()
            self._last_pose = (position, quaternion)
        except Exception:
            logger.warning("Failed to read Airbot Play end pose. Using cached values.", exc_info=True)
            if self._last_pose is not None:
                position, quaternion = self._last_pose
            else:
                position = np.full(3, np.nan, dtype=float)
                quaternion = np.array([0.0, 0.0, 0.0, 1.0], dtype=float)

        rotvec = _quat_xyzw_to_rotvec(quaternion)
        observation.update(
            {
                "ee.x": float(position[0]),
                "ee.y": float(position[1]),
                "ee.z": float(position[2]),
                "ee.wx": float(rotvec[0]),
                "ee.wy": float(rotvec[1]),
                "ee.wz": float(rotvec[2]),
            }
        )

        try:
            gripper_pos = self._read_gripper_pos()
            self._last_gripper_pos = gripper_pos
        except Exception:
            logger.warning("Failed to read Airbot Play gripper state. Using cached value.", exc_info=True)
            gripper_pos = self._last_gripper_pos if self._last_gripper_pos is not None else np.nan
        observation["ee.gripper_pos"] = float(gripper_pos)

        try:
            joint_positions = self._read_state_vector("get_joint_pos", "pos")
            self._last_joint_positions = joint_positions
        except Exception:
            logger.warning("Failed to read Airbot Play joint positions. Using cached values.", exc_info=True)
            joint_positions = self._fallback_vector(self._last_joint_positions)

        try:
            joint_velocities = self._read_state_vector("get_joint_vel", "vel")
            self._last_joint_velocities = joint_velocities
        except Exception:
            logger.warning("Failed to read Airbot Play joint velocities. Using cached values.", exc_info=True)
            joint_velocities = self._fallback_vector(self._last_joint_velocities)

        for joint_idx in range(1, self.config.arm_dof + 1):
            observation[f"joint_{joint_idx}.pos"] = float(joint_positions[joint_idx - 1])
            observation[f"joint_{joint_idx}.vel"] = float(joint_velocities[joint_idx - 1])

        for name, camera in self.cameras.items():
            camera_key = f"observation.images.{name}"
            try:
                observation[camera_key] = camera.read_latest()
            except Exception:
                logger.warning("Failed to read Airbot camera '%s'. Returning a blank frame.", name, exc_info=True)
                observation[camera_key] = self._blank_image(name)

        return observation

    @check_if_not_connected
    def send_action(self, action: RobotAction) -> RobotAction:
        self._validate_action(action)

        target_position = np.array([float(action[key]) for key in POSITION_KEYS], dtype=float)
        target_rotvec = np.array([float(action[key]) for key in ROTATION_KEYS], dtype=float)
        target_gripper = self._clip_gripper(float(action["ee.gripper_pos"]))

        safe_position = self._clip_position(target_position)
        target_quaternion = _rotvec_to_quat_xyzw(target_rotvec)
        target_pose = self._make_cartesian_pose(tuple(safe_position.tolist()), tuple(target_quaternion.tolist()))

        sent_action: RobotAction = {
            "ee.x": float(safe_position[0]),
            "ee.y": float(safe_position[1]),
            "ee.z": float(safe_position[2]),
            "ee.wx": float(target_rotvec[0]),
            "ee.wy": float(target_rotvec[1]),
            "ee.wz": float(target_rotvec[2]),
            "ee.gripper_pos": target_gripper,
        }

        self._send_pose_with_gripper(target_pose, target_gripper)

        self._last_pose = (safe_position, target_quaternion)
        self._last_gripper_pos = target_gripper
        self._last_action = sent_action
        return sent_action

    @check_if_not_connected
    def reset_to_home(self) -> None:
        result = self._call_client_method(("return_zero",), [((), {})])
        if result is _MISSING:
            raise AttributeError("Airbot Play SDK does not expose `return_zero()`.")
        if result is False:
            raise RuntimeError("Airbot Play failed to reset to the home pose.")

        self._last_pose = None
        self._last_joint_positions = None
        self._last_joint_velocities = None
        self._last_gripper_pos = None
        self._last_action = None
        self._prime_state_cache()

    def return_zero(self) -> None:
        self.reset_to_home()

    @check_if_not_connected
    def disconnect(self) -> None:
        if self.config.disconnect_return_zero:
            try:
                self.reset_to_home()
            except Exception:
                logger.warning("Failed to reset Airbot Play during disconnect.", exc_info=True)

        for name, camera in self.cameras.items():
            try:
                camera.disconnect()
            except Exception:  # nosec B110
                logger.warning("Failed to disconnect Airbot camera '%s'.", name, exc_info=True)

        self._best_effort_release_client()
        logger.info("%s disconnected.", self)

    def _make_client(self) -> Any:
        if AirbotClient is None:
            raise ImportError("`arm_sdk` is not importable in the current environment.")

        constructors = [
            ((), {"host": self.config.host, "port": self.config.port, "arm_dof": self.config.arm_dof}),
            ((self.config.host, self.config.port, self.config.arm_dof), {}),
            ((), {"host": self.config.host, "port": self.config.port}),
            ((self.config.host, self.config.port), {}),
        ]
        last_type_error: TypeError | None = None
        for args, kwargs in constructors:
            try:
                return AirbotClient(*args, **kwargs)
            except TypeError as error:
                last_type_error = error

        if last_type_error is None:
            raise RuntimeError("Failed to construct Airbot Play client.")
        raise last_type_error

    def _acquire_control(self) -> bool:
        result = self._call_client_method(
            ("acquire_control",),
            [
                ((), {"lease_ms": self.config.lease_ms, "renew_period_s": self.config.renew_period_s}),
                ((self.config.lease_ms, self.config.renew_period_s), {}),
                ((), {"lease_ms": self.config.lease_ms}),
                ((), {}),
            ],
        )
        if result is _MISSING:
            logger.debug("Airbot Play SDK has no explicit acquire_control(); assuming no lease is needed.")
            return True
        return result is not False

    def _set_controller_mode(self) -> None:
        result = self._call_client_method(
            ("switch_mode", "set_control_mode"),
            [
                ((self.config.controller_mode,), {}),
                ((), {"mode": self.config.controller_mode}),
                ((), {"controller_mode": self.config.controller_mode}),
            ],
        )
        if result is _MISSING:
            if self.config.controller_mode != "servo_control":
                raise RuntimeError(
                    "Airbot Play SDK does not expose a control-mode switch method, "
                    f"so controller_mode={self.config.controller_mode!r} cannot be applied."
                )
            logger.debug("Airbot Play SDK has no explicit control-mode switch; assuming servo control is already active.")
            return
        if result is False:
            raise RuntimeError(f"Failed to switch Airbot Play into {self.config.controller_mode!r}.")

    def _set_speed_if_requested(self, method_names: tuple[str, ...], value: float | None) -> None:
        if value is None:
            return

        result = self._call_client_method(
            method_names,
            [
                ((value,), {}),
                ((), {"value": value}),
                ((), {"speed": value}),
            ],
        )
        if result is _MISSING:
            logger.warning("Airbot Play SDK has no %s() method; skipping speed setup.", method_names[0])
            return
        if result is False:
            raise RuntimeError(f"Failed to apply Airbot Play setting via {method_names[0]}({value}).")

    def _prime_state_cache(self) -> None:
        try:
            self._last_pose = self._read_end_pose()
        except Exception:  # nosec B110
            logger.debug("Could not prime Airbot Play end-pose cache.", exc_info=True)
        try:
            self._last_joint_positions = self._read_state_vector("get_joint_pos", "pos")
        except Exception:  # nosec B110
            logger.debug("Could not prime Airbot Play joint-position cache.", exc_info=True)
        try:
            self._last_joint_velocities = self._read_state_vector("get_joint_vel", "vel")
        except Exception:  # nosec B110
            logger.debug("Could not prime Airbot Play joint-velocity cache.", exc_info=True)
        try:
            self._last_gripper_pos = self._read_gripper_pos()
        except Exception:  # nosec B110
            logger.debug("Could not prime Airbot Play gripper cache.", exc_info=True)

    def _read_end_pose(self) -> tuple[np.ndarray, np.ndarray]:
        result = self._call_client_method(("get_end_pose",), [((), {})])
        if result is _MISSING or result is None:
            raise AttributeError("Airbot Play SDK does not expose `get_end_pose()`.")

        if isinstance(result, dict):
            position = result.get("position")
            orientation = result.get("orientation")
        else:
            position = getattr(result, "position", None)
            orientation = getattr(result, "orientation", None)
            if position is None or orientation is None:
                if isinstance(result, (list, tuple)) and len(result) == 2:
                    position, orientation = result

        position_array = np.asarray(position, dtype=float).reshape(-1)
        orientation_array = np.asarray(orientation, dtype=float).reshape(-1)
        if position_array.size != 3:
            raise ValueError(f"Expected 3D end-effector position, got shape {position_array.shape}.")
        if orientation_array.size != 4:
            raise ValueError(f"Expected quaternion in xyzw order, got shape {orientation_array.shape}.")
        return position_array[:3], orientation_array[:4]

    def _read_gripper_pos(self) -> float:
        method_names = ("get_eef_pos", "get_gripper_pos", "get_eef_position")
        result = self._call_client_method(method_names, [((), {})])
        if result is _MISSING or result is None:
            raise AttributeError("Airbot Play SDK does not expose a gripper position getter.")
        return float(result)

    def _read_state_vector(self, method_name: str, kind: str) -> np.ndarray:
        result = self._call_client_method((method_name,), [((), {})])
        if result is _MISSING or result is None:
            raise AttributeError(f"Airbot Play SDK does not expose `{method_name}()`.")
        return self._normalize_state_vector(result, kind)

    def _normalize_state_vector(self, values: Any, kind: str) -> np.ndarray:
        if isinstance(values, dict):
            ordered_values = []
            for joint_idx in range(self.config.arm_dof):
                candidates = [
                    joint_idx,
                    joint_idx + 1,
                    f"joint_{joint_idx + 1}",
                    f"joint_{joint_idx + 1}.{kind}",
                ]
                for candidate in candidates:
                    if candidate in values:
                        ordered_values.append(float(values[candidate]))
                        break
                else:
                    raise KeyError(f"Missing joint_{joint_idx + 1} in Airbot Play {kind} state.")
            return np.asarray(ordered_values, dtype=float)

        array = np.asarray(values, dtype=float).reshape(-1)
        if array.size < self.config.arm_dof:
            raise ValueError(
                f"Expected at least {self.config.arm_dof} joint {kind} values, got {array.size}."
            )
        return array[: self.config.arm_dof]

    def _fallback_vector(self, cached_vector: np.ndarray | None) -> np.ndarray:
        if cached_vector is not None:
            return cached_vector
        return np.full(self.config.arm_dof, np.nan, dtype=float)

    def _clip_position(self, position: np.ndarray) -> np.ndarray:
        safe_position = np.asarray(position, dtype=float).copy()

        if self._last_pose is not None and self.config.max_position_step_m is not None:
            if isinstance(self.config.max_position_step_m, dict):
                max_steps = self.config.max_position_step_m
            else:
                max_steps = {key: float(self.config.max_position_step_m) for key in POSITION_KEYS}

            reference_position = self._last_pose[0]
            for index, key in enumerate(POSITION_KEYS):
                max_step = max_steps[key]
                delta = safe_position[index] - reference_position[index]
                safe_position[index] = reference_position[index] + float(np.clip(delta, -max_step, max_step))

        if self.config.position_limits is not None:
            for index, key in enumerate(POSITION_KEYS):
                if key in self.config.position_limits:
                    low, high = self.config.position_limits[key]
                    safe_position[index] = float(np.clip(safe_position[index], low, high))

        return safe_position

    def _clip_gripper(self, gripper_pos: float) -> float:
        low, high = self.config.gripper_limits
        return float(np.clip(gripper_pos, low, high))

    def _send_pose_with_gripper(self, target_pose: Any, gripper_target: float) -> None:
        if self.config.enable_gripper and self._gripper_control_mode == "embedded_with_move_end_pose":
            embedded_options, embedded_supported = self._build_control_options(gripper_target)
            if embedded_supported:
                try:
                    if self._move_end_pose(target_pose, embedded_options):
                        return
                except Exception:
                    logger.warning(
                        "Embedded Airbot gripper control failed. Falling back to separate servo.",
                        exc_info=True,
                    )
            self._gripper_control_mode = "separate_servo"

        base_options, _ = self._build_control_options()
        if not self._move_end_pose(target_pose, base_options):
            raise RuntimeError("Airbot Play rejected the Cartesian action command.")

        if self.config.enable_gripper and self._gripper_control_mode == "separate_servo":
            self._move_gripper_separately(gripper_target)

    def _build_control_options(self, gripper_target: float | None = None) -> tuple[Any, bool]:
        if ArmControlOptions is None:
            raise ImportError("`arm_sdk` did not expose `ArmControlOptions`.")

        options = ArmControlOptions()
        self._try_setattr(options, "blocking", self.config.blocking)
        if self.config.default_arm_eff is not None:
            self._try_setattr(options, "arm_eff", self.config.default_arm_eff)
        if self.config.default_eef_eff is not None:
            self._try_setattr(options, "eef_eff", self.config.default_eef_eff)

        embedded_supported = True
        if gripper_target is not None:
            embedded_supported = self._try_setattr(options, "eef_pos", gripper_target)
        return options, embedded_supported

    def _move_end_pose(self, target_pose: Any, options: Any) -> bool:
        result = self._call_client_method(
            ("move_end_pose", "move_to_cart_pose", "servo_cart_pose"),
            [
                ((), {"pos": target_pose, "options": options, "timeout_ms": self.config.timeout_ms}),
                ((), {"cart_pose": target_pose, "options": options, "timeout_ms": self.config.timeout_ms}),
                ((target_pose,), {"options": options, "timeout_ms": self.config.timeout_ms}),
                ((target_pose, options, self.config.timeout_ms), {}),
                ((target_pose, options), {}),
                ((target_pose,), {}),
            ],
        )
        if result is _MISSING:
            raise AttributeError("Airbot Play SDK does not expose `move_end_pose()`.")
        return result is not False

    def _move_gripper_separately(self, gripper_target: float) -> None:
        result = self._call_client_method(
            ("servo_eef_pos", "move_eef_pos", "move_eff_pos"),
            [
                ((), {"pos": gripper_target, "timeout_ms": self.config.timeout_ms}),
                ((), {"eef_pos": gripper_target, "timeout_ms": self.config.timeout_ms}),
                ((gripper_target,), {"timeout_ms": self.config.timeout_ms}),
                ((gripper_target,), {}),
            ],
        )
        if result is _MISSING:
            raise RuntimeError(
                "Embedded Airbot gripper control failed, and no separate gripper servo method is available."
            )
        if result is False:
            raise RuntimeError("Airbot Play rejected the separate gripper command.")

    def _make_cartesian_pose(self, position: tuple[float, float, float], orientation: tuple[float, float, float, float]):
        if CartesianPose is None:
            raise ImportError("`arm_sdk` did not expose `CartesianPose`.")
        return CartesianPose(position, orientation)

    def _call_client_method(
        self,
        method_names: tuple[str, ...],
        call_specs: list[tuple[tuple[Any, ...], dict[str, Any]]],
    ) -> Any:
        client = self._require_client()
        for method_name in method_names:
            method = getattr(client, method_name, None)
            if not callable(method):
                continue

            last_type_error: TypeError | None = None
            for args, kwargs in call_specs:
                try:
                    return method(*args, **kwargs)
                except TypeError as error:
                    last_type_error = error
            if last_type_error is not None:
                raise last_type_error
        return _MISSING

    def _require_client(self) -> Any:
        if self._client is None:
            raise ConnectionError("Airbot Play client is not initialized.")
        return self._client

    def _best_effort_release_client(self) -> None:
        client = self._client
        if client is not None and self._lease_acquired:
            try:
                self._call_client_method(("release_control",), [((), {})])
            except Exception:  # nosec B110
                logger.warning("Failed to release Airbot Play control lease.", exc_info=True)

        if client is not None:
            for method_name in ("close", "disconnect"):
                method = getattr(client, method_name, None)
                if callable(method):
                    try:
                        method()
                    except Exception:  # nosec B110
                        logger.warning("Failed to call Airbot Play client.%s().", method_name, exc_info=True)
                    break

        self._lease_acquired = False
        self._client = None
        self._is_connected = False

    def _validate_action(self, action: RobotAction) -> None:
        missing_keys = [key for key in EE_ACTION_KEYS if key not in action]
        extra_keys = sorted(set(action) - set(EE_ACTION_KEYS))
        if not missing_keys and not extra_keys:
            return

        parts: list[str] = []
        if missing_keys:
            parts.append(f"missing keys: {missing_keys}")
        if extra_keys:
            parts.append(f"unexpected keys: {extra_keys}")
        raise KeyError(f"Invalid Airbot Play action; {'; '.join(parts)}.")

    def _blank_image(self, camera_name: str) -> np.ndarray:
        camera_config = self.config.cameras[camera_name]
        return np.zeros((camera_config.height, camera_config.width, 3), dtype=np.uint8)

    @staticmethod
    def _camera_is_connected(camera: Any) -> bool:
        is_connected = getattr(camera, "is_connected", True)
        if callable(is_connected):
            return bool(is_connected())
        return bool(is_connected)

    @staticmethod
    def _try_setattr(obj: Any, attr_name: str, value: Any) -> bool:
        try:
            setattr(obj, attr_name, value)
            return True
        except Exception:  # nosec B110
            logger.debug("ArmControlOptions has no '%s' attribute.", attr_name, exc_info=True)
            return False
