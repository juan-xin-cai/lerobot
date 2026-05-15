from typing import Any

from lerobot.types import RobotAction

from ..teleoperator import Teleoperator
from ..utils import make_teleoperator_from_config
from .config_composite import CompositeTeleoperatorConfig


class CompositeTeleoperator(Teleoperator):
    """A teleoperator that merges actions from multiple child teleoperators."""

    config_class = CompositeTeleoperatorConfig
    name = "composite"

    def __init__(self, config: CompositeTeleoperatorConfig):
        super().__init__(config)
        self.config = config
        self.devices = {
            name: make_teleoperator_from_config(device_config)
            for name, device_config in config.devices.items()
        }
        self._is_connected = False

    @property
    def action_features(self) -> dict[str, type]:
        features: dict[str, type] = {}
        for device in self.devices.values():
            overlap = set(features) & set(device.action_features)
            if overlap:
                raise ValueError(f"Composite teleoperator action feature overlap: {sorted(overlap)}")
            features.update(device.action_features)
        return features

    @property
    def feedback_features(self) -> dict:
        return {}

    @property
    def is_connected(self) -> bool:
        return self._is_connected and all(device.is_connected for device in self.devices.values())

    @property
    def is_calibrated(self) -> bool:
        return all(device.is_calibrated for device in self.devices.values())

    def connect(self, calibrate: bool = True) -> None:
        for device in self.devices.values():
            device.connect(calibrate=calibrate)
        self._is_connected = True

    def calibrate(self) -> None:
        for device in self.devices.values():
            device.calibrate()

    def configure(self) -> None:
        for device in self.devices.values():
            device.configure()

    def get_action(self) -> RobotAction:
        action: RobotAction = {}
        for device in self.devices.values():
            device_action = device.get_action()
            overlap = set(action) & set(device_action)
            if overlap:
                raise ValueError(f"Composite teleoperator action value overlap: {sorted(overlap)}")
            action.update(device_action)
        return action

    def send_feedback(self, feedback: dict[str, Any]) -> None:
        for device in self.devices.values():
            device.send_feedback(feedback)

    def disconnect(self) -> None:
        for device in reversed(list(self.devices.values())):
            if device.is_connected:
                device.disconnect()
        self._is_connected = False
