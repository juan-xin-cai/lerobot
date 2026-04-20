# Airbot Play VR

这一组脚本把 VR 发包端、`AirbotVRTeleoperator`、`AirbotPlayRobot` 串成一条可联调、可录制的数据链。

## 1. 依赖

先装 LeRobot 侧依赖：

```bash
uv sync --locked --extra airbot_play --extra test
```

再安装 AIRBOT 官方提供的 Python SDK wheel。这个 wheel 需要提供 `arm_sdk` 模块；如果没有，`AirbotPlayRobot.connect()` 会直接报错。

## 2. VR 发包端

Windows 侧 `SteamMode.cpp` 现在发的是单行 JSON 协议，每行一帧，默认要连到 Python 侧的 `teleop.port`。

建议把端口统一成 `5006`，例如：

```bash
uv run python /Users/roy/udexreal/lerobot/examples/airbot_play_vr/teleoperate.py \
  --robot.host=127.0.0.1 \
  --robot.port=50051 \
  --teleop.host=0.0.0.0 \
  --teleop.port=5006 \
  --fps=30
```

## 3. 遥操作

最小联调命令：

```bash
uv run python /Users/roy/udexreal/lerobot/examples/airbot_play_vr/teleoperate.py \
  --robot.host=127.0.0.1 \
  --robot.port=50051 \
  --teleop.port=5006 \
  --teleop.packet_timeout_s=0.25 \
  --display_data=true
```

当前按键语义：

- `Trigger`：开始或停止跟随
- `B`：触发 `reset_robot` 与 `recenter_vr`

## 4. 录制

录制脚本会：

- 把机器人实际发送的动作写进主 `action`
- 额外写入 `observation.teleop_vr`
- 在收到 `reset_robot` 时暂停当前动作下发，执行 `robot.reset_to_home()`

示例：

```bash
uv run python /Users/roy/udexreal/lerobot/examples/airbot_play_vr/record.py \
  --robot.host=127.0.0.1 \
  --robot.port=50051 \
  --teleop.port=5006 \
  --dataset.repo_id=your-hf-user/airbot-play-vr \
  --dataset.single_task="Teleoperate the Airbot Play arm with a VR controller." \
  --dataset.num_episodes=5 \
  --dataset.episode_time_s=60 \
  --dataset.reset_time_s=20 \
  --dataset.video=true
```

键盘控制沿用通用录制逻辑：

- `→`：提前结束当前阶段
- `←`：重录当前 episode
- `Esc`：停止整轮录制

## 5. 联调清单

建议按这个顺序验：

1. 启动 Airbot gRPC 服务端，确认 `robot.host:robot.port` 可达。
2. 启动 VR 发包端，抓包确认 TCP 每行一帧 JSON。
3. 运行 `teleoperate.py`，确认 `Trigger` 跟随、`B` 复位能走通。
4. 运行 `record.py`，确认相机图像、`observation.state`、`action`、`observation.teleop_vr` 都有写入。
5. 重复执行 2 到 4，确认断流后 Teleoperator 会保持最后安全动作，不会突然跳变。

## 6. `options.eef_pos` 真机确认

这一项还不能在本仓库里自动确认，需要真机逐档验证。建议按 5 档做：

| 档位 | `eef_pos` |
| --- | --- |
| 1 | `0.00` |
| 2 | `0.25` |
| 3 | `0.50` |
| 4 | `0.75` |
| 5 | `1.00` |

每一档都记录下面四件事：

1. 夹爪实际开口方向是否与数值增大一致。
2. 回读值是否跟目标值同量纲。
3. 边界外的数值会不会被截断。
4. `move_end_pose(..., options.eef_pos=...)` 与独立 `move_eef(...)` 的表现是否一致。

如果 `eef_pos` 的单位、方向或边界和当前假设不一致，录制链路要把 `gripper_control_mode` 切到 `separate_servo`，并把这里的结论补回 plan 文档。
