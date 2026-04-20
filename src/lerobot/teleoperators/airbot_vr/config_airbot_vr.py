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

from dataclasses import dataclass

from ..config import TeleoperatorConfig


@TeleoperatorConfig.register_subclass("airbot_vr")
@dataclass(kw_only=True)
class AirbotVRTeleoperatorConfig(TeleoperatorConfig):
    id: str = "airbot_vr"
    host: str = "0.0.0.0"
    port: int = 5006
    packet_timeout_s: float = 0.5
    startup_wait_s: float = 5.0
    default_gripper_pos: float = 0.0
    max_position_jump_m: float = 0.2
    accept_tracking_lost: bool = False

    def __post_init__(self) -> None:
        if not self.host:
            raise ValueError("host must not be empty.")
        if not 1 <= self.port <= 65535:
            raise ValueError("port must be in [1, 65535].")
        if self.packet_timeout_s <= 0:
            raise ValueError("packet_timeout_s must be > 0.")
        if self.startup_wait_s <= 0:
            raise ValueError("startup_wait_s must be > 0.")
        if self.max_position_jump_m < 0:
            raise ValueError("max_position_jump_m must be >= 0.")
