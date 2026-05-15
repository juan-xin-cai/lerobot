from dataclasses import dataclass

from ..config import TeleoperatorConfig


@TeleoperatorConfig.register_subclass("steamvr_tracker")
@dataclass
class SteamVRTrackerTeleoperatorConfig(TeleoperatorConfig):
    """Receive SteamVR tracker samples forwarded by an external PC over TCP."""

    host: str = "0.0.0.0"
    port: int = 8765
    expected_hz: float = 120.0
    timeout_s: float = 0.05
    wait_for_connection: bool = True
    wait_for_first_sample: bool = True
    log_raw_tcp_until_first_sample: bool = True
    raw_tcp_log_interval_s: float = 1.0
    recv_buffer_size: int = 4096
