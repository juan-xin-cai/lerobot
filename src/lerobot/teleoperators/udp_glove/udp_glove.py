import json
import logging
import socket
import threading
import time
from dataclasses import dataclass
from typing import Any

from lerobot.types import RobotAction
from lerobot.utils.decorators import check_if_already_connected, check_if_not_connected

from ..teleoperator import Teleoperator
from .config_udp_glove import UDPGloveTeleoperatorConfig

logger = logging.getLogger(__name__)

GLOVE_FIELD_MEANINGS = (
    "thumb_third_joint_pitch",
    "thumb_second_joint_pitch",
    "thumb_first_joint_pitch",
    "thumb_first_joint_yaw",
    "index_third_joint_pitch",
    "index_second_joint_pitch",
    "index_first_joint_pitch",
    "index_first_joint_yaw",
    "middle_third_joint_pitch",
    "middle_second_joint_pitch",
    "middle_first_joint_pitch",
    "middle_first_joint_yaw",
    "ring_third_joint_pitch",
    "ring_second_joint_pitch",
    "ring_first_joint_pitch",
    "ring_first_joint_yaw",
    "pinky_third_joint_pitch",
    "pinky_second_joint_pitch",
    "pinky_first_joint_pitch",
    "pinky_first_joint_yaw",
    "thumb_first_joint_roll",
    "index_first_joint_roll",
    "pinky_first_joint_roll",
    "gesture_reserved",
    "imu_quaternion_w",
    "imu_quaternion_x",
    "imu_quaternion_y",
    "imu_quaternion_z",
)

RAW_TO_ACTION_FIELD = {
    **{f"l{i}": f"left_{meaning}" for i, meaning in enumerate(GLOVE_FIELD_MEANINGS)},
    **{f"r{i}": f"right_{meaning}" for i, meaning in enumerate(GLOVE_FIELD_MEANINGS)},
}
RAW_GLOVE_FIELDS = tuple(RAW_TO_ACTION_FIELD)
ACTION_GLOVE_FIELDS = tuple(RAW_TO_ACTION_FIELD.values())


@dataclass(frozen=True)
class UDPGloveSample:
    values: dict[str, float]
    receive_time_s: float
    seq: int


def parse_udp_glove_packet(packet: str, receive_time_s: float, seq: int) -> UDPGloveSample:
    payload = json.loads(packet)
    parameters = _find_parameter_list(payload)

    values_by_name: dict[str, float] = {}
    for item in parameters:
        try:
            name = item["Name"]
            value = item["Value"]
        except (KeyError, TypeError) as e:
            raise ValueError(f"Invalid glove parameter item: {item!r}") from e

        if name in RAW_TO_ACTION_FIELD:
            values_by_name[RAW_TO_ACTION_FIELD[name]] = float(value)

    missing = [name for name in RAW_GLOVE_FIELDS if RAW_TO_ACTION_FIELD[name] not in values_by_name]
    if missing:
        logger.debug("UDP glove packet missing fields, filling with 0.0: %s", missing)
        for name in missing:
            values_by_name[RAW_TO_ACTION_FIELD[name]] = 0.0

    return UDPGloveSample(values=values_by_name, receive_time_s=receive_time_s, seq=seq)


def _find_parameter_list(payload: Any) -> list[dict[str, Any]]:
    if not isinstance(payload, dict):
        raise ValueError("UDP glove packet must be a JSON object")

    udhand = payload.get("Udhand")
    if isinstance(udhand, dict) and isinstance(udhand.get("Parameter"), list):
        return udhand["Parameter"]

    for role_payload in payload.values():
        if isinstance(role_payload, dict) and isinstance(role_payload.get("Parameter"), list):
            return role_payload["Parameter"]

    raise ValueError("UDP glove packet must contain a role object with Parameter")


class UDPGloveTeleoperator(Teleoperator):
    """UDP receiver for Udhand glove JSON parameters."""

    config_class = UDPGloveTeleoperatorConfig
    name = "udp_glove"

    def __init__(self, config: UDPGloveTeleoperatorConfig):
        super().__init__(config)
        self.config = config
        self._is_connected = False
        self._stop_event = threading.Event()
        self._first_sample_event = threading.Event()
        self._thread: threading.Thread | None = None
        self._socket: socket.socket | None = None
        self._lock = threading.Lock()
        self._latest_sample: UDPGloveSample | None = None
        self._seq = 0
        self._last_sample_log_t = 0.0
        self._last_raw_log_t = 0.0

    @property
    def action_features(self) -> dict[str, type]:
        features = {f"glove.{name}": float for name in ACTION_GLOVE_FIELDS}
        features.update(
            {
                "glove_receive_time": float,
                "glove_seq": float,
                "glove_valid": float,
            }
        )
        return features

    @property
    def feedback_features(self) -> dict:
        return {}

    @property
    def is_connected(self) -> bool:
        return self._is_connected

    @property
    def is_calibrated(self) -> bool:
        return True

    @check_if_already_connected
    def connect(self, calibrate: bool = True) -> None:
        self._stop_event.clear()
        self._first_sample_event.clear()
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._socket.bind((self.config.host, self.config.port))
        self._socket.settimeout(0.2)
        self._thread = threading.Thread(target=self._receive_loop, name=f"{self.name}-udp", daemon=True)
        self._thread.start()
        self._is_connected = True
        logger.info("%s listening for UDP glove samples on %s:%s", self, self.config.host, self.config.port)

        if self.config.wait_for_first_sample:
            logger.info("%s waiting for first valid UDP glove sample...", self)
            self._first_sample_event.wait()
            logger.info("%s received first valid UDP glove sample.", self)

    def calibrate(self) -> None:
        return None

    def configure(self) -> None:
        return None

    @check_if_not_connected
    def get_action(self) -> RobotAction:
        with self._lock:
            sample = self._latest_sample

        if sample is None:
            return self._action_from_values({name: 0.0 for name in ACTION_GLOVE_FIELDS}, 0.0, 0.0, False)

        is_fresh = time.perf_counter() - sample.receive_time_s <= self.config.timeout_s
        return self._action_from_values(
            sample.values,
            sample.receive_time_s,
            float(sample.seq),
            is_fresh,
        )

    def send_feedback(self, feedback: dict[str, Any]) -> None:
        return None

    @check_if_not_connected
    def disconnect(self) -> None:
        self._stop_event.set()
        self._first_sample_event.set()
        if self._socket is not None:
            self._socket.close()
            self._socket = None
        if self._thread is not None:
            self._thread.join(timeout=1.0)
            self._thread = None
        self._is_connected = False
        logger.info("%s disconnected.", self)

    def _receive_loop(self) -> None:
        while not self._stop_event.is_set():
            try:
                assert self._socket is not None
                data, addr = self._socket.recvfrom(self.config.recv_buffer_size)
            except socket.timeout:
                continue
            except OSError:
                break

            receive_time_s = time.perf_counter()
            packet = data.decode("utf-8", errors="replace").strip()
            self._log_raw_udp(packet, reason=f"recv from {addr}")
            self._store_packet(packet, receive_time_s)

    def _store_packet(self, packet: str, receive_time_s: float) -> None:
        self._seq += 1
        try:
            sample = parse_udp_glove_packet(packet, receive_time_s, self._seq)
        except (json.JSONDecodeError, ValueError) as e:
            self._log_raw_udp(packet, reason="parse_failed", force=True)
            logger.warning("%s ignored malformed UDP glove packet: %s", self, e)
            return

        with self._lock:
            self._latest_sample = sample
        self._first_sample_event.set()

        if receive_time_s - self._last_sample_log_t >= 1.0:
            self._last_sample_log_t = receive_time_s
            logger.info(
                "%s sample seq=%d l0=%.3f l27=%.3f r0=%.3f r27=%.3f",
                self,
                sample.seq,
                sample.values["left_thumb_third_joint_pitch"],
                sample.values["left_imu_quaternion_z"],
                sample.values["right_thumb_third_joint_pitch"],
                sample.values["right_imu_quaternion_z"],
            )

    def _log_raw_udp(self, text: str, *, reason: str, force: bool = False) -> None:
        if not self.config.log_raw_udp_until_first_sample:
            return
        if self._first_sample_event.is_set():
            return

        now = time.perf_counter()
        if not force and now - self._last_raw_log_t < self.config.raw_udp_log_interval_s:
            return

        self._last_raw_log_t = now
        if len(text) <= 1000:
            logger.info("%s raw udp %s len=%d: %r", self, reason, len(text), text)
            return

        logger.info(
            "%s raw udp %s len=%d head=%r tail=%r",
            self,
            reason,
            len(text),
            text[:500],
            text[-500:],
        )

    @staticmethod
    def _action_from_values(
        values: dict[str, float], receive_time_s: float, seq: float, valid: bool
    ) -> RobotAction:
        action = {f"glove.{name}": values[name] for name in ACTION_GLOVE_FIELDS}
        action.update(
            {
                "glove_receive_time": receive_time_s,
                "glove_seq": seq,
                "glove_valid": float(valid),
            }
        )
        return action
