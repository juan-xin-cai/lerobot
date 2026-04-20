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

import json
import logging
import socket
import threading
import time
from typing import Any

import numpy as np

from lerobot.types import RobotAction
from lerobot.utils.decorators import check_if_already_connected, check_if_not_connected
from lerobot.utils.rotation import Rotation

from ..teleoperator import Teleoperator
from ..utils import TeleopEvents
from .config_airbot_vr import AirbotVRTeleoperatorConfig

logger = logging.getLogger(__name__)

_BUFFER_SIZE = 4096
_SOCKET_TIMEOUT_S = 0.1
_CUSTOM_EVENT_KEYS = ("reset_robot", "recenter_vr")
_ACTION_FEATURES = {
    "ee.x": float,
    "ee.y": float,
    "ee.z": float,
    "ee.wx": float,
    "ee.wy": float,
    "ee.wz": float,
    "ee.gripper_pos": float,
}


def _close_socket(sock: socket.socket | None) -> None:
    if sock is None:
        return

    try:
        sock.shutdown(socket.SHUT_RDWR)
    except OSError:
        pass

    try:
        sock.close()
    except OSError:
        pass


class AirbotVRTeleoperator(Teleoperator):
    config_class = AirbotVRTeleoperatorConfig
    name = "airbot_vr"

    def __init__(self, config: AirbotVRTeleoperatorConfig):
        super().__init__(config)
        self.config = config
        self._state_lock = threading.Lock()
        self._stop_event = threading.Event()
        self._first_valid_packet_event = threading.Event()
        self._server_socket: socket.socket | None = None
        self._client_socket: socket.socket | None = None
        self._receiver_thread: threading.Thread | None = None
        self._latest_packet: dict[str, Any] | None = None
        self._last_packet_time: float | None = None
        self._last_safe_action: RobotAction | None = None
        self._latest_vr_auxiliary_data = self._make_default_vr_auxiliary_data()
        self._pending_events = self._make_empty_custom_events()
        self._raw_event_states = self._make_empty_custom_events()

    @property
    def action_features(self) -> dict[str, type]:
        return dict(_ACTION_FEATURES)

    @property
    def feedback_features(self) -> dict[str, type]:
        return {}

    @property
    def is_connected(self) -> bool:
        return self._receiver_thread is not None and self._receiver_thread.is_alive()

    @property
    def is_calibrated(self) -> bool:
        return True

    @property
    def latest_vr_auxiliary_data(self) -> dict[str, float | bool]:
        with self._state_lock:
            return dict(self._latest_vr_auxiliary_data)

    def get_vr_auxiliary_data(self) -> dict[str, float | bool]:
        return self.latest_vr_auxiliary_data

    @check_if_already_connected
    def connect(self, calibrate: bool = True) -> None:
        del calibrate
        self._reset_runtime_state()
        self._stop_event.clear()
        self._first_valid_packet_event.clear()

        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind((self.config.host, self.config.port))
        server_socket.listen(1)
        server_socket.settimeout(_SOCKET_TIMEOUT_S)
        self._server_socket = server_socket

        self._receiver_thread = threading.Thread(
            target=self._receive_loop,
            name="airbot-vr-recv",
            daemon=True,
        )
        self._receiver_thread.start()

        if not self._first_valid_packet_event.wait(timeout=self.config.startup_wait_s):
            self.disconnect()
            raise TimeoutError(
                f"No valid Airbot VR packet received within {self.config.startup_wait_s:.2f}s."
            )

    def calibrate(self) -> None:
        pass

    def configure(self) -> None:
        pass

    @check_if_not_connected
    def get_action(self) -> RobotAction:
        with self._state_lock:
            if self._last_safe_action is None:
                raise RuntimeError("AirbotVRTeleoperator has not received a valid action packet yet.")
            last_safe_action = dict(self._last_safe_action)
            latest_packet = self._latest_packet
            last_packet_time = self._last_packet_time

        if self._is_packet_stale(last_packet_time):
            return last_safe_action

        if latest_packet is not None and not self._packet_allows_motion(latest_packet):
            return last_safe_action

        return last_safe_action

    def get_teleop_events(self) -> dict[str, Any]:
        base_events: dict[str, Any] = {
            TeleopEvents.IS_INTERVENTION: False,
            TeleopEvents.TERMINATE_EPISODE: False,
            TeleopEvents.SUCCESS: False,
            TeleopEvents.RERECORD_EPISODE: False,
            **self._make_empty_custom_events(),
        }
        if not self.is_connected:
            return base_events

        with self._state_lock:
            custom_events = dict(self._pending_events)
            self._pending_events = self._make_empty_custom_events()

        base_events.update(custom_events)
        base_events[TeleopEvents.IS_INTERVENTION] = any(custom_events.values())
        return base_events

    def send_feedback(self, feedback: dict[str, Any]) -> None:
        raise NotImplementedError("AirbotVRTeleoperator does not implement feedback.")

    def disconnect(self) -> None:
        self._stop_event.set()
        self._close_client_socket()
        _close_socket(self._server_socket)
        self._server_socket = None

        if self._receiver_thread is not None:
            self._receiver_thread.join(timeout=max(1.0, self.config.packet_timeout_s))
            self._receiver_thread = None

        self._first_valid_packet_event.clear()
        self._reset_runtime_state()

    @staticmethod
    def _make_empty_custom_events() -> dict[str, bool]:
        return {key: False for key in _CUSTOM_EVENT_KEYS}

    @staticmethod
    def _quat_wxyz_to_rotvec(quat_wxyz: np.ndarray) -> np.ndarray:
        quat_xyzw = np.array([quat_wxyz[1], quat_wxyz[2], quat_wxyz[3], quat_wxyz[0]], dtype=float)
        return Rotation.from_quat(quat_xyzw).as_rotvec()

    def _make_default_vr_auxiliary_data(self) -> dict[str, float | bool]:
        return {
            "teleop.vr.pos.x": 0.0,
            "teleop.vr.pos.y": 0.0,
            "teleop.vr.pos.z": 0.0,
            "teleop.vr.quat.w": 1.0,
            "teleop.vr.quat.x": 0.0,
            "teleop.vr.quat.y": 0.0,
            "teleop.vr.quat.z": 0.0,
            "teleop.vr.trigger": 0.0,
            "teleop.vr.grip": self.config.default_gripper_pos,
            "teleop.vr.tracking_ok": False,
        }

    def _reset_runtime_state(self) -> None:
        with self._state_lock:
            self._latest_packet = None
            self._last_packet_time = None
            self._last_safe_action = None
            self._latest_vr_auxiliary_data = self._make_default_vr_auxiliary_data()
            self._pending_events = self._make_empty_custom_events()
            self._raw_event_states = self._make_empty_custom_events()

    def _close_client_socket(self) -> None:
        with self._state_lock:
            client_socket = self._client_socket
            self._client_socket = None
        _close_socket(client_socket)

    def _receive_loop(self) -> None:
        buffer = b""
        while not self._stop_event.is_set():
            if self._client_socket is None:
                accepted = self._accept_client()
                if accepted:
                    buffer = b""
                continue

            client_socket = self._client_socket
            if client_socket is None:
                continue

            try:
                chunk = client_socket.recv(_BUFFER_SIZE)
            except socket.timeout:
                continue
            except OSError:
                self._close_client_socket()
                buffer = b""
                continue

            if not chunk:
                self._close_client_socket()
                buffer = b""
                continue

            buffer += chunk
            while b"\n" in buffer:
                raw_line, buffer = buffer.split(b"\n", 1)
                line = raw_line.strip()
                if not line:
                    continue
                self._handle_packet_line(line)

    def _accept_client(self) -> bool:
        server_socket = self._server_socket
        if server_socket is None:
            return False

        try:
            client_socket, _ = server_socket.accept()
        except socket.timeout:
            return False
        except OSError:
            return False

        client_socket.settimeout(_SOCKET_TIMEOUT_S)
        with self._state_lock:
            previous_client = self._client_socket
            self._client_socket = client_socket
        _close_socket(previous_client)
        return True

    def _handle_packet_line(self, line: bytes) -> None:
        try:
            packet = json.loads(line.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as exc:
            logger.warning("Discarding malformed Airbot VR packet: %s", exc)
            return

        if not isinstance(packet, dict):
            logger.warning("Discarding Airbot VR packet that is not a JSON object.")
            return

        try:
            action = self._packet_to_action(packet)
            auxiliary_data = self._packet_to_vr_auxiliary_data(packet)
            custom_events = self._extract_custom_events(packet)
        except (TypeError, ValueError, KeyError) as exc:
            logger.warning("Discarding invalid Airbot VR packet: %s", exc)
            return

        received_at = time.monotonic()
        with self._state_lock:
            self._latest_packet = packet
            self._last_packet_time = received_at
            self._latest_vr_auxiliary_data = auxiliary_data
            self._update_pending_events(custom_events)
            if self._last_safe_action is None and not bool(packet.get("following", True)):
                self._last_safe_action = action
                self._first_valid_packet_event.set()

            if not self._packet_allows_motion(packet):
                return
            if self._is_position_jump_too_large(action):
                return

            self._last_safe_action = action
            self._first_valid_packet_event.set()

    def _extract_pose(self, packet: dict[str, Any]) -> tuple[np.ndarray, np.ndarray]:
        pose = packet["pose"]
        if not isinstance(pose, dict):
            raise TypeError("pose must be a JSON object.")

        position = np.asarray(pose["pos"], dtype=float)
        quat_wxyz = np.asarray(pose["quat_wxyz"], dtype=float)
        if position.shape != (3,):
            raise ValueError("pose.pos must have exactly 3 elements.")
        if quat_wxyz.shape != (4,):
            raise ValueError("pose.quat_wxyz must have exactly 4 elements.")

        return position, quat_wxyz

    def _packet_to_action(self, packet: dict[str, Any]) -> RobotAction:
        position, quat_wxyz = self._extract_pose(packet)
        analog = packet.get("analog") or {}
        if not isinstance(analog, dict):
            raise TypeError("analog must be a JSON object.")

        rotvec = self._quat_wxyz_to_rotvec(quat_wxyz)
        gripper_pos = float(analog.get("grip", self.config.default_gripper_pos))

        return {
            "ee.x": float(position[0]),
            "ee.y": float(position[1]),
            "ee.z": float(position[2]),
            "ee.wx": float(rotvec[0]),
            "ee.wy": float(rotvec[1]),
            "ee.wz": float(rotvec[2]),
            "ee.gripper_pos": gripper_pos,
        }

    def _packet_to_vr_auxiliary_data(self, packet: dict[str, Any]) -> dict[str, float | bool]:
        position, quat_wxyz = self._extract_pose(packet)
        analog = packet.get("analog") or {}
        if not isinstance(analog, dict):
            raise TypeError("analog must be a JSON object.")

        return {
            "teleop.vr.pos.x": float(position[0]),
            "teleop.vr.pos.y": float(position[1]),
            "teleop.vr.pos.z": float(position[2]),
            "teleop.vr.quat.w": float(quat_wxyz[0]),
            "teleop.vr.quat.x": float(quat_wxyz[1]),
            "teleop.vr.quat.y": float(quat_wxyz[2]),
            "teleop.vr.quat.z": float(quat_wxyz[3]),
            "teleop.vr.trigger": float(analog.get("trigger", 0.0)),
            "teleop.vr.grip": float(analog.get("grip", self.config.default_gripper_pos)),
            "teleop.vr.tracking_ok": bool(packet.get("tracking_ok", True)),
        }

    def _extract_custom_events(self, packet: dict[str, Any]) -> dict[str, bool]:
        events = packet.get("events") or {}
        if not isinstance(events, dict):
            raise TypeError("events must be a JSON object.")

        return {
            "reset_robot": bool(events.get("reset_robot", False)),
            "recenter_vr": bool(events.get("recenter_vr", False)),
        }

    def _update_pending_events(self, custom_events: dict[str, bool]) -> None:
        for event_name, is_active in custom_events.items():
            if is_active and not self._raw_event_states[event_name]:
                self._pending_events[event_name] = True
            self._raw_event_states[event_name] = is_active

    def _packet_allows_motion(self, packet: dict[str, Any]) -> bool:
        tracking_ok = bool(packet.get("tracking_ok", True))
        following = bool(packet.get("following", True))
        return following and (tracking_ok or self.config.accept_tracking_lost)

    def _is_position_jump_too_large(self, action: RobotAction) -> bool:
        if self._last_safe_action is None or self.config.max_position_jump_m == 0:
            return False

        current_position = np.array([action["ee.x"], action["ee.y"], action["ee.z"]], dtype=float)
        previous_position = np.array(
            [
                self._last_safe_action["ee.x"],
                self._last_safe_action["ee.y"],
                self._last_safe_action["ee.z"],
            ],
            dtype=float,
        )
        return np.linalg.norm(current_position - previous_position) > self.config.max_position_jump_m

    def _is_packet_stale(self, last_packet_time: float | None) -> bool:
        return last_packet_time is None or (
            time.monotonic() - last_packet_time
        ) > self.config.packet_timeout_s
