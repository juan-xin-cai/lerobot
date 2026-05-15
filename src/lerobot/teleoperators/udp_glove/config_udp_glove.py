from dataclasses import dataclass

from ..config import TeleoperatorConfig


@TeleoperatorConfig.register_subclass("udp_glove")
@dataclass
class UDPGloveTeleoperatorConfig(TeleoperatorConfig):
    """Receive Udhand glove samples over UDP JSON."""

    host: str = "0.0.0.0"
    port: int = 9001
    timeout_s: float = 0.2
    recv_buffer_size: int = 65535
    wait_for_first_sample: bool = True
    log_raw_udp_until_first_sample: bool = True
    raw_udp_log_interval_s: float = 1.0
