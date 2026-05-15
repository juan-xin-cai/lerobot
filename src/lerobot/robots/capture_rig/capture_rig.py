import logging
import time
from functools import cached_property

from lerobot.cameras import make_cameras_from_configs
from lerobot.types import RobotAction, RobotObservation
from lerobot.utils.decorators import check_if_already_connected, check_if_not_connected

from ..robot import Robot
from .config_capture_rig import CaptureRigConfig

logger = logging.getLogger(__name__)


class CaptureRig(Robot):
    """A robot-compatible wrapper for camera-only data collection."""

    config_class = CaptureRigConfig
    name = "capture_rig"

    def __init__(self, config: CaptureRigConfig):
        super().__init__(config)
        self.config = config
        self.cameras = make_cameras_from_configs(config.cameras)
        self._is_connected = False

    @cached_property
    def observation_features(self) -> dict[str, tuple]:
        return {
            cam: (self.config.cameras[cam].height, self.config.cameras[cam].width, 3)
            for cam in self.cameras
        }

    @cached_property
    def action_features(self) -> dict[str, type]:
        return {}

    @property
    def is_connected(self) -> bool:
        return self._is_connected and all(cam.is_connected for cam in self.cameras.values())

    @check_if_already_connected
    def connect(self, calibrate: bool = True) -> None:
        for cam in self.cameras.values():
            cam.connect()

        self.configure()
        self._is_connected = True
        if self.cameras:
            logger.info(f"{self} connected with {len(self.cameras)} camera(s).")
        else:
            logger.info(f"{self} connected without cameras.")

    @property
    def is_calibrated(self) -> bool:
        return True

    def calibrate(self) -> None:
        return None

    def configure(self) -> None:
        return None

    @check_if_not_connected
    def get_observation(self) -> RobotObservation:
        obs: RobotObservation = {}
        for cam_key, cam in self.cameras.items():
            start = time.perf_counter()
            obs[cam_key] = cam.read_latest()
            dt_ms = (time.perf_counter() - start) * 1e3
            logger.debug(f"{self} read {cam_key}: {dt_ms:.1f}ms")
        return obs

    @check_if_not_connected
    def send_action(self, action: RobotAction) -> RobotAction:
        return {}

    @check_if_not_connected
    def disconnect(self) -> None:
        for cam in self.cameras.values():
            cam.disconnect()
        self._is_connected = False
        logger.info(f"{self} disconnected.")
