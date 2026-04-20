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

import json
import socket
import threading
import time

import pytest

from lerobot.teleoperators.airbot_vr import AirbotVRTeleoperator, AirbotVRTeleoperatorConfig
from lerobot.teleoperators.utils import TeleopEvents


def _pick_free_port() -> int:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("127.0.0.1", 0))
    port = sock.getsockname()[1]
    sock.close()
    return port


def _base_packet() -> dict:
    return {
        "version": 1,
        "timestamp_ms": 1713599999999,
        "frame": "airbot_base",
        "tracking_ok": True,
        "following": True,
        "pose": {
            "pos": [0.4, 0.1, 0.2],
            "quat_wxyz": [0.70710678, 0.0, 0.0, 0.70710678],
        },
        "analog": {
            "trigger": 0.9,
            "grip": 0.3,
            "joystick_x": 0.0,
            "joystick_y": 0.0,
        },
        "buttons": {
            "trigger": True,
            "grip": False,
            "a": False,
            "b": False,
            "menu": False,
            "joystick_press": False,
        },
        "events": {
            "reset_robot": False,
            "recenter_vr": False,
            "rerecord_episode": False,
            "terminate_episode": False,
        },
    }


def _start_sender(port: int, packets: list[dict], delay_s: float = 0.05) -> threading.Thread:
    def _send():
        deadline = time.time() + 5.0
        while True:
            try:
                with socket.create_connection(("127.0.0.1", port), timeout=0.5) as conn:
                    time.sleep(delay_s)
                    for packet in packets:
                        conn.sendall((json.dumps(packet) + "\n").encode("utf-8"))
                        time.sleep(delay_s)
                    return
            except OSError:
                if time.time() >= deadline:
                    raise
                time.sleep(0.05)

    thread = threading.Thread(target=_send, daemon=True)
    thread.start()
    return thread


def _start_raw_sender(port: int, chunks: list[bytes], delay_s: float = 0.05) -> threading.Thread:
    def _send():
        deadline = time.time() + 5.0
        while True:
            try:
                with socket.create_connection(("127.0.0.1", port), timeout=0.5) as conn:
                    time.sleep(delay_s)
                    for chunk in chunks:
                        conn.sendall(chunk)
                        time.sleep(delay_s)
                    return
            except OSError:
                if time.time() >= deadline:
                    raise
                time.sleep(0.05)

    thread = threading.Thread(target=_send, daemon=True)
    thread.start()
    return thread


@pytest.fixture
def teleop():
    config = AirbotVRTeleoperatorConfig(
        host="127.0.0.1",
        port=_pick_free_port(),
        packet_timeout_s=0.05,
        startup_wait_s=2.0,
        max_position_jump_m=0.2,
        accept_tracking_lost=False,
    )
    device = AirbotVRTeleoperator(config)
    yield device
    if device.is_connected:
        device.disconnect()


def test_default_config_rejects_tracking_lost_packets():
    assert AirbotVRTeleoperatorConfig().accept_tracking_lost is False


def test_connect_and_get_action_rotvec(teleop):
    sender = _start_sender(teleop.config.port, [_base_packet()])
    teleop.connect()
    action = teleop.get_action()

    assert pytest.approx(action["ee.x"]) == 0.4
    assert pytest.approx(action["ee.y"]) == 0.1
    assert pytest.approx(action["ee.z"]) == 0.2
    assert pytest.approx(action["ee.wx"], abs=1e-6) == 0.0
    assert pytest.approx(action["ee.wy"], abs=1e-6) == 0.0
    assert pytest.approx(action["ee.wz"], abs=1e-5) == pytest.approx(1.57079633, abs=1e-5)
    assert pytest.approx(action["ee.gripper_pos"]) == 0.3
    sender.join(timeout=1.0)


def test_connect_accepts_idle_first_packet(teleop):
    packet = _base_packet()
    packet["following"] = False
    packet["tracking_ok"] = False
    sender = _start_sender(teleop.config.port, [packet])
    teleop.connect()
    action = teleop.get_action()

    assert action["ee.x"] == pytest.approx(0.4)
    assert action["ee.gripper_pos"] == pytest.approx(0.3)
    sender.join(timeout=1.0)


def test_tracking_lost_following_first_packet_waits_for_safe_packet():
    config = AirbotVRTeleoperatorConfig(
        host="127.0.0.1",
        port=_pick_free_port(),
        packet_timeout_s=1.0,
        startup_wait_s=2.0,
        max_position_jump_m=0.2,
    )
    device = AirbotVRTeleoperator(config)
    unsafe_first_packet = _base_packet()
    unsafe_first_packet["tracking_ok"] = False
    unsafe_first_packet["following"] = True
    unsafe_first_packet["pose"]["pos"] = [0.8, 0.8, 0.8]
    safe_second_packet = _base_packet()
    sender = _start_sender(config.port, [unsafe_first_packet, safe_second_packet], delay_s=0.03)

    try:
        device.connect()
        action = device.get_action()
    finally:
        if device.is_connected:
            device.disconnect()

    assert action["ee.x"] == pytest.approx(0.4)
    assert action["ee.y"] == pytest.approx(0.1)
    assert action["ee.z"] == pytest.approx(0.2)
    sender.join(timeout=1.0)


def test_connect_times_out_without_first_packet():
    config = AirbotVRTeleoperatorConfig(
        host="127.0.0.1",
        port=_pick_free_port(),
        packet_timeout_s=0.05,
        startup_wait_s=0.1,
    )
    device = AirbotVRTeleoperator(config)

    try:
        with pytest.raises(TimeoutError):
            device.connect()
        assert not device.is_connected
    finally:
        if device.is_connected:
            device.disconnect()


def test_stale_packet_falls_back_to_last_action(teleop):
    packet = _base_packet()
    sender = _start_sender(teleop.config.port, [packet])
    teleop.connect()
    first = teleop.get_action()
    time.sleep(teleop.config.packet_timeout_s + 0.05)
    second = teleop.get_action()

    assert second == first
    sender.join(timeout=1.0)


def test_position_jump_keeps_last_safe_action():
    config = AirbotVRTeleoperatorConfig(
        host="127.0.0.1",
        port=_pick_free_port(),
        packet_timeout_s=1.0,
        startup_wait_s=2.0,
        max_position_jump_m=0.2,
    )
    device = AirbotVRTeleoperator(config)
    first_packet = _base_packet()
    second_packet = _base_packet()
    second_packet["pose"]["pos"] = [0.8, 0.8, 0.8]
    sender = _start_sender(config.port, [first_packet, second_packet], delay_s=0.03)

    try:
        device.connect()
        first = device.get_action()
        time.sleep(0.08)
        second = device.get_action()
    finally:
        if device.is_connected:
            device.disconnect()

    assert second == first
    sender.join(timeout=1.0)


def test_tracking_lost_keeps_last_safe_action(teleop):
    first_packet = _base_packet()
    second_packet = _base_packet()
    second_packet["tracking_ok"] = False
    second_packet["pose"]["pos"] = [0.8, 0.8, 0.8]
    sender = _start_sender(teleop.config.port, [first_packet, second_packet], delay_s=0.03)
    teleop.connect()
    first = teleop.get_action()
    time.sleep(0.1)
    second = teleop.get_action()

    assert second == first
    sender.join(timeout=1.0)


def test_malformed_and_empty_lines_are_ignored_before_valid_packet(teleop):
    valid_line = (json.dumps(_base_packet()) + "\n").encode("utf-8")
    sender = _start_raw_sender(
        teleop.config.port,
        [b"\n", b"{not-json}\n", b"[]\n", valid_line],
        delay_s=0.03,
    )
    teleop.connect()
    action = teleop.get_action()

    assert action["ee.x"] == pytest.approx(0.4)
    sender.join(timeout=1.0)


def test_split_packet_is_buffered_until_newline(teleop):
    line = (json.dumps(_base_packet()) + "\n").encode("utf-8")
    sender = _start_raw_sender(
        teleop.config.port,
        [line[:8], line[8:35], line[35:]],
        delay_s=0.03,
    )
    teleop.connect()
    action = teleop.get_action()

    assert action["ee.y"] == pytest.approx(0.1)
    sender.join(timeout=1.0)


def test_sticky_packets_are_split_by_newline(teleop):
    first_packet = _base_packet()
    second_packet = _base_packet()
    second_packet["pose"]["pos"] = [0.45, 0.1, 0.2]
    payload = (json.dumps(first_packet) + "\n" + json.dumps(second_packet) + "\n").encode("utf-8")
    sender = _start_raw_sender(teleop.config.port, [payload])
    teleop.connect()
    time.sleep(0.05)
    action = teleop.get_action()

    assert action["ee.x"] == pytest.approx(0.45)
    sender.join(timeout=1.0)


def test_events_are_consumed_once(teleop):
    packet = _base_packet()
    packet["events"]["reset_robot"] = True
    packet["events"]["recenter_vr"] = True
    sender = _start_sender(teleop.config.port, [packet])
    teleop.connect()
    events = teleop.get_teleop_events()
    next_events = teleop.get_teleop_events()

    assert events["reset_robot"] is True
    assert events["recenter_vr"] is True
    assert events[TeleopEvents.IS_INTERVENTION] is True
    assert next_events["reset_robot"] is False
    assert next_events["recenter_vr"] is False
    sender.join(timeout=1.0)


def test_vr_auxiliary_data_exposes_expected_fields(teleop):
    first_packet = _base_packet()
    second_packet = _base_packet()
    second_packet["tracking_ok"] = False
    sender = _start_sender(teleop.config.port, [first_packet, second_packet], delay_s=0.03)
    teleop.connect()
    time.sleep(0.08)
    aux = teleop.get_vr_auxiliary_data()

    assert aux["teleop.vr.pos.x"] == pytest.approx(0.4)
    assert aux["teleop.vr.quat.w"] == pytest.approx(0.70710678)
    assert aux["teleop.vr.trigger"] == pytest.approx(0.9)
    assert aux["teleop.vr.grip"] == pytest.approx(0.3)
    assert aux["teleop.vr.tracking_ok"] == 0.0
    sender.join(timeout=1.0)
