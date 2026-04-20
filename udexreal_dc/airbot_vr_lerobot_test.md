# Airbot Play VR 测试报告

> 关联 Spec：未发现独立 `spec.md`，本次严格按 Plan 测试  
> 关联 Plan：`udexreal_dc/airbot_vr_lerobot_technical_plan.md`  
> 测试日期：2026-04-20  
> 测试环境：macOS Darwin 25.4.0 arm64；临时 venv `/tmp/lerobot_airbot_test_venv`；Python 3.12.13；pytest 9.0.3；torch 2.10.0；torchvision 0.25.0；numpy 2.4.4；scipy 1.17.1；opencv-python-headless 4.13.0；draccus 0.8.0

## 测试总结

| 指标 | 数值 |
| ---- | ---- |
| 单元测试总数 | 22 |
| 单元测试通过 | 22 |
| 单元测试失败 | 0 |
| 自动化场景验证总数 | 3 |
| 自动化场景验证通过 | 3 |
| 自动化场景验证失败 | 0 |
| 已执行项总体通过率 | 100% |

## 单元测试详情

### ✅ 通过的测试

| 测试文件 | 测试用例 | 对应 TODO |
| -------- | -------- | --------- |
| `tests/teleoperators/test_airbot_vr.py` | `test_default_config_rejects_tracking_lost_packets` | TODO-T1 / TODO-S3 |
| `tests/teleoperators/test_airbot_vr.py` | `test_connect_and_get_action_rotvec` | TODO-T1 / TODO-S3 |
| `tests/teleoperators/test_airbot_vr.py` | `test_connect_accepts_idle_first_packet` | TODO-T1 / TODO-S3 |
| `tests/teleoperators/test_airbot_vr.py` | `test_tracking_lost_following_first_packet_waits_for_safe_packet` | TODO-T1 / TODO-S3 |
| `tests/teleoperators/test_airbot_vr.py` | `test_connect_times_out_without_first_packet` | TODO-T1 / TODO-S3 |
| `tests/teleoperators/test_airbot_vr.py` | `test_stale_packet_falls_back_to_last_action` | TODO-T1 / TODO-S3 |
| `tests/teleoperators/test_airbot_vr.py` | `test_position_jump_keeps_last_safe_action` | TODO-T1 / TODO-S3 |
| `tests/teleoperators/test_airbot_vr.py` | `test_tracking_lost_keeps_last_safe_action` | TODO-T1 / TODO-S3 |
| `tests/teleoperators/test_airbot_vr.py` | `test_malformed_and_empty_lines_are_ignored_before_valid_packet` | TODO-T1 / TODO-S3 |
| `tests/teleoperators/test_airbot_vr.py` | `test_split_packet_is_buffered_until_newline` | TODO-T1 / TODO-S3 |
| `tests/teleoperators/test_airbot_vr.py` | `test_sticky_packets_are_split_by_newline` | TODO-T1 / TODO-S3 |
| `tests/teleoperators/test_airbot_vr.py` | `test_events_are_consumed_once` | TODO-T1 / TODO-S4 |
| `tests/teleoperators/test_airbot_vr.py` | `test_vr_auxiliary_data_exposes_expected_fields` | TODO-T1 / TODO-C2 |
| `tests/robots/test_airbot_play.py` | `test_config_defaults_and_registration` | TODO-T2 / TODO-S5 |
| `tests/robots/test_airbot_play.py` | `test_connect_disconnect` | TODO-T2 / TODO-S6 |
| `tests/robots/test_airbot_play.py` | `test_connect_cleans_up_when_camera_connection_fails` | TODO-T2 / TODO-S6 |
| `tests/robots/test_airbot_play.py` | `test_get_observation_returns_plan_fields` | TODO-T2 / TODO-S7 |
| `tests/robots/test_airbot_play.py` | `test_send_action_converts_rotvec_to_quaternion_and_embeds_gripper` | TODO-T2 / TODO-S8 |
| `tests/robots/test_airbot_play.py` | `test_send_action_falls_back_to_separate_gripper_servo_when_embedded_move_fails` | TODO-T2 / TODO-S8 / TODO-T4 |
| `tests/robots/test_airbot_play.py` | `test_send_action_rejects_missing_fields` | TODO-T2 / TODO-S8 |
| `tests/robots/test_airbot_play.py` | `test_reset_to_home_calls_return_zero` | TODO-T2 / TODO-S9 |
| `tests/robots/test_airbot_play.py` | `test_missing_arm_sdk_raises_clear_error` | TODO-T2 / TODO-S5 |

### ❌ 失败的测试

无。

## 集成 / 场景验证详情

### 场景 1：Airbot VR/Robot 自动化测试

- **操作步骤**：执行 `PYTHONPATH=src /tmp/lerobot_airbot_test_venv/bin/python -m pytest tests/teleoperators/test_airbot_vr.py tests/robots/test_airbot_play.py -q`
- **期望结果**：Teleoperator 和 Robot 单元测试全部通过
- **实际结果**：✅ 符合预期，`22 passed in 16.83s`
- **证据**：

```text
Testing with DEVICE='mps'
......................                                                   [100%]
22 passed in 16.83s
```

### 场景 2：SteamMode C++ 编译检查

- **操作步骤**：执行 `g++ -std=c++17 -Iudexreal_dc/assistant4/embodied/include -Iudexreal_dc/assistant4/embodied/SteamMode -c udexreal_dc/assistant4/embodied/SteamMode/SteamMode.cpp -o /tmp/airbot_test_steam_mode.o`
- **期望结果**：C++ 侧新增协议发送、B 键复位相关代码可编译
- **实际结果**：✅ 符合预期，命令退出码为 0
- **证据**：命令无错误输出，生成 `/tmp/airbot_test_steam_mode.o`

### 场景 3：Python 语法编译检查

- **操作步骤**：执行 `PYTHONPATH=src /tmp/lerobot_airbot_test_venv/bin/python -m py_compile src/lerobot/teleoperators/airbot_vr/airbot_vr.py src/lerobot/teleoperators/airbot_vr/config_airbot_vr.py src/lerobot/robots/airbot_play/airbot_play.py src/lerobot/robots/airbot_play/config_airbot_play.py tests/teleoperators/test_airbot_vr.py tests/robots/test_airbot_play.py`
- **期望结果**：Python 侧实现和测试文件均可编译
- **实际结果**：✅ 符合预期，命令退出码为 0
- **证据**：命令无错误输出

## 未覆盖的测试场景

- Plan 场景 1「最小遥操作联调」未执行：当前环境没有 Airbot gRPC 服务端、SteamMode 运行实例和右手 VR 手柄。
- Plan 场景 2「夹爪联调」未执行：需要真机确认握把输入和夹爪动作方向、限位、同步延迟。
- Plan 场景 3「录制单轮 episode」未执行：需要相机、Airbot、VR 输入和实际数据集写入路径。
- Plan 场景 4「复位流程」未执行：需要录制过程中触发真实 `reset_robot` 并观察机械臂回零。
- Plan 场景 5「异常断连」未执行：需要运行中的 SteamMode 或网络链路，当前只用单元测试覆盖了断流保持最后安全动作。
- TODO-T4 未完成：`options.eef_pos` 的真实语义仍需在 Airbot 实机 SDK 上确认。

## 遗留问题

- 仓库本机缺正式 `uv` 环境，本次使用 `/tmp/lerobot_airbot_test_venv` 临时安装依赖执行测试；建议后续在项目标准环境中重跑 `uv sync --locked --extra test --extra dev` 后的同一组测试。
- 当前自动化测试已覆盖 Plan 的单元测试标准，但硬件场景仍需人工联调确认，尤其是夹爪语义、复位后的控制抢占、异常断连时机械臂实际保持效果。
