from dataclasses import dataclass, field

from ..config import TeleoperatorConfig


@TeleoperatorConfig.register_subclass("composite")
@dataclass
class CompositeTeleoperatorConfig(TeleoperatorConfig):
    """Combine multiple teleoperators into one action source."""

    devices: dict[str, TeleoperatorConfig] = field(default_factory=dict)
