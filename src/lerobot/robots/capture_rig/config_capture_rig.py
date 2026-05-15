from dataclasses import dataclass, field

from lerobot.cameras import CameraConfig

from ..config import RobotConfig


@RobotConfig.register_subclass("capture_rig")
@dataclass
class CaptureRigConfig(RobotConfig):
    """Camera-only capture rig for embodiment-free data collection."""

    cameras: dict[str, CameraConfig] = field(default_factory=dict)
