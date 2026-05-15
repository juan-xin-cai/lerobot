import logging
import re
import socket
import threading
import time
from dataclasses import dataclass
from datetime import datetime
from typing import Any

from lerobot.types import RobotAction
from lerobot.utils.decorators import check_if_already_connected, check_if_not_connected

from ..teleoperator import Teleoperator
from .config_steamvr_tracker import SteamVRTrackerTeleoperatorConfig

logger = logging.getLogger(__name__)

_PACKET_RE = re.compile(
    r"(?P<timestamp>\d{4}-\d{1,2}-\d{1,2} \d{1,2}:\d{1,2}:\d{1,2}:\d{1,3});.*?"
    r"LPos:\s*\((?P<lpos>[^)]*)\)\s*LRot:\s*\((?P<lrot>[^)]*)\).*?"
    r"RPos:\s*\((?P<rpos>[^)]*)\)\s*RRot:\s*\((?P<rrot>[^)]*)\)",
    re.DOTALL,
)


@dataclass(frozen=True)
class SteamVRTrackerSample:
    left_pos: tuple[float, float, float]
    left_rot: tuple[float, float, float]
    right_pos: tuple[float, float, float]
    right_rot: tuple[float, float, float]
    source_time_s: float
    receive_time_s: float
    seq: int


def parse_steamvr_tracker_packet(packet: str, receive_time_s: float, seq: int) -> SteamVRTrackerSample:
    match = _PACKET_RE.search(packet)
    if match is None:
        raise ValueError(f"Could not parse SteamVR tracker packet: {packet!r}")

    def parse_vec3(raw: str) -> tuple[float, float, float]:
        parts = [float(part.strip()) for part in raw.split(",")]
        if len(parts) != 3:
            raise ValueError(f"Expected 3 position values, got {len(parts)} in {raw!r}")
        return parts[0], parts[1], parts[2]

    date_part, time_part = match.group("timestamp").split(" ")
    year, month, day = (int(part) for part in date_part.split("-"))
    hour, minute, second, millisecond = (int(part) for part in time_part.split(":"))
    source_dt = datetime(year, month, day, hour, minute, second, millisecond * 1000)
    source_time_s = (
        source_dt.hour * 60 * 60
        + source_dt.minute * 60
        + source_dt.second
        + source_dt.microsecond / 1_000_000
    )
    return SteamVRTrackerSample(
        left_pos=parse_vec3(match.group("lpos")),
        left_rot=parse_vec3(match.group("lrot")),
        right_pos=parse_vec3(match.group("rpos")),
        right_rot=parse_vec3(match.group("rrot")),
        source_time_s=source_time_s,
        receive_time_s=receive_time_s,
        seq=seq,
    )


class SteamVRTrackerTeleoperator(Teleoperator):
    """TCP receiver for externally forwarded SteamVR tracker positions."""

    config_class = SteamVRTrackerTeleoperatorConfig
    name = "steamvr_tracker"

    def __init__(self, config: SteamVRTrackerTeleoperatorConfig):
        super().__init__(config)
        self.config = config
        self._is_connected = False
        self._stop_event = threading.Event()
        self._client_connected_event = threading.Event()
        self._first_sample_event = threading.Event()
        self._thread: threading.Thread | None = None
        self._server_socket: socket.socket | None = None
        self._lock = threading.Lock()
        self._latest_sample: SteamVRTrackerSample | None = None
        self._seq = 0
        self._last_sample_log_t = 0.0
        self._last_raw_log_t = 0.0

    @property
    def action_features(self) -> dict[str, type]:
        return {
            "left_wrist_tracker.x": float,
            "left_wrist_tracker.y": float,
            "left_wrist_tracker.z": float,
            "left_wrist_tracker.rot_x": float,
            "left_wrist_tracker.rot_y": float,
            "left_wrist_tracker.rot_z": float,
            "right_wrist_tracker.x": float,
            "right_wrist_tracker.y": float,
            "right_wrist_tracker.z": float,
            "right_wrist_tracker.rot_x": float,
            "right_wrist_tracker.rot_y": float,
            "right_wrist_tracker.rot_z": float,
            "tracker_source_time": float,
            "tracker_receive_time": float,
            "tracker_seq": float,
            "tracker_valid": float,
        }

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
        if self.config.expected_hz <= 0:
            raise ValueError("expected_hz must be positive.")

        self._stop_event.clear()
        self._client_connected_event.clear()
        self._first_sample_event.clear()
        self._server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._server_socket.bind((self.config.host, self.config.port))
        self._server_socket.listen(1)
        self._server_socket.settimeout(0.2)
        self._thread = threading.Thread(target=self._receive_loop, name=f"{self.name}-tcp", daemon=True)
        self._thread.start()
        self._is_connected = True
        logger.info(
            "%s listening on %s:%s for %.1f Hz SteamVR tracker samples",
            self,
            self.config.host,
            self.config.port,
            self.config.expected_hz,
        )
        if self.config.wait_for_connection:
            logger.info("%s waiting for SteamVR tracker TCP client connection...", self)
            self._client_connected_event.wait()
            logger.info("%s SteamVR tracker TCP client connected.", self)

        if self.config.wait_for_first_sample:
            logger.info("%s waiting for first valid SteamVR tracker sample...", self)
            self._first_sample_event.wait()
            logger.info("%s received first valid SteamVR tracker sample.", self)

    def calibrate(self) -> None:
        return None

    def configure(self) -> None:
        return None

    @check_if_not_connected
    def get_action(self) -> RobotAction:
        with self._lock:
            sample = self._latest_sample

        if sample is None:
            return self._action_from_values(
                (0.0, 0.0, 0.0),
                (0.0, 0.0, 0.0),
                (0.0, 0.0, 0.0),
                (0.0, 0.0, 0.0),
                0.0,
                0.0,
                0.0,
                False,
            )

        is_fresh = time.perf_counter() - sample.receive_time_s <= self.config.timeout_s
        return self._action_from_values(
            sample.left_pos,
            sample.left_rot,
            sample.right_pos,
            sample.right_rot,
            sample.source_time_s,
            sample.receive_time_s,
            float(sample.seq),
            is_fresh,
        )

    def send_feedback(self, feedback: dict[str, Any]) -> None:
        return None

    @check_if_not_connected
    def disconnect(self) -> None:
        self._stop_event.set()
        self._client_connected_event.set()
        self._first_sample_event.set()
        if self._server_socket is not None:
            self._server_socket.close()
            self._server_socket = None
        if self._thread is not None:
            self._thread.join(timeout=1.0)
            self._thread = None
        self._is_connected = False
        logger.info("%s disconnected.", self)

    def _receive_loop(self) -> None:
        while not self._stop_event.is_set():
            try:
                assert self._server_socket is not None
                conn, addr = self._server_socket.accept()
            except socket.timeout:
                continue
            except OSError:
                break

            logger.info("%s accepted SteamVR tracker TCP connection from %s", self, addr)
            self._client_connected_event.set()
            with conn:
                conn.settimeout(0.2)
                self._read_connection(conn)

    def _read_connection(self, conn: socket.socket) -> None:
        buffer = ""
        while not self._stop_event.is_set():
            try:
                chunk = conn.recv(self.config.recv_buffer_size)
            except socket.timeout:
                continue
            except OSError:
                break
            if not chunk:
                break

            raw_text = chunk.decode("utf-8", errors="replace")
            self._log_raw_tcp(raw_text, reason="recv")
            buffer += raw_text
            lines = buffer.splitlines(keepends=False)
            if buffer and not buffer.endswith(("\n", "\r")):
                buffer = lines.pop() if lines else buffer
            else:
                buffer = ""

            for line in lines:
                self._store_packet(line)

            if "\n" not in buffer and "\r" not in buffer:
                match = _PACKET_RE.search(buffer)
                if match is not None:
                    self._store_packet(match.group(0))
                    buffer = buffer[match.end() :]

    def _store_packet(self, packet: str) -> None:
        receive_time_s = time.perf_counter()
        self._seq += 1
        try:
            sample = parse_steamvr_tracker_packet(packet, receive_time_s, self._seq)
        except ValueError:
            self._log_raw_tcp(packet, reason="parse_failed", force=True)
            logger.debug("%s ignored malformed SteamVR tracker packet: %r", self, packet)
            return

        with self._lock:
            self._latest_sample = sample
        self._first_sample_event.set()

        if receive_time_s - self._last_sample_log_t >= 1.0:
            self._last_sample_log_t = receive_time_s
            logger.info(
                "%s sample seq=%d LPos=%s LRot=%s RPos=%s RRot=%s",
                self,
                sample.seq,
                sample.left_pos,
                sample.left_rot,
                sample.right_pos,
                sample.right_rot,
            )

    def _log_raw_tcp(self, text: str, *, reason: str, force: bool = False) -> None:
        if not self.config.log_raw_tcp_until_first_sample:
            return
        if self._first_sample_event.is_set():
            return

        now = time.perf_counter()
        if not force and now - self._last_raw_log_t < self.config.raw_tcp_log_interval_s:
            return

        self._last_raw_log_t = now
        logger.info("%s raw tcp %s: %r", self, reason, text[:500])

    @staticmethod
    def _action_from_values(
        left_pos: tuple[float, float, float],
        left_rot: tuple[float, float, float],
        right_pos: tuple[float, float, float],
        right_rot: tuple[float, float, float],
        source_time_s: float,
        receive_time_s: float,
        seq: float,
        valid: bool,
    ) -> RobotAction:
        return {
            "left_wrist_tracker.x": left_pos[0],
            "left_wrist_tracker.y": left_pos[1],
            "left_wrist_tracker.z": left_pos[2],
            "left_wrist_tracker.rot_x": left_rot[0],
            "left_wrist_tracker.rot_y": left_rot[1],
            "left_wrist_tracker.rot_z": left_rot[2],
            "right_wrist_tracker.x": right_pos[0],
            "right_wrist_tracker.y": right_pos[1],
            "right_wrist_tracker.z": right_pos[2],
            "right_wrist_tracker.rot_x": right_rot[0],
            "right_wrist_tracker.rot_y": right_rot[1],
            "right_wrist_tracker.rot_z": right_rot[2],
            "tracker_source_time": source_time_s,
            "tracker_receive_time": receive_time_s,
            "tracker_seq": seq,
            "tracker_valid": float(valid),
        }
