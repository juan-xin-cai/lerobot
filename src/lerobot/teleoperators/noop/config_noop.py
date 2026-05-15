from dataclasses import dataclass

from ..config import TeleoperatorConfig


@TeleoperatorConfig.register_subclass("noop")
@dataclass
class NoOpTeleoperatorConfig(TeleoperatorConfig):
    """Teleoperator that produces no actions."""
