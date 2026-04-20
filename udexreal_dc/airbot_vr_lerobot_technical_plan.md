# Airbot Play VR 遥操作与 LeRobot 采集链路实施计划

> 关联输入资料：
> - `/Users/roy/udexreal/lerobot/udexreal_dc/assistant4/embodied/SteamMode/SteamMode.cpp`
> - `/Users/roy/udexreal/lerobot/udexreal_dc/assistant4/embodied/SteamMode/arm_comm.cpp`
> - `/Users/roy/udexreal/lerobot/udexreal_dc/vr_connect_arm.py`
> - `/Users/roy/udexreal/lerobot/src/lerobot/teleoperators/teleoperator.py`
> - `/Users/roy/udexreal/lerobot/src/lerobot/robots/robot.py`
> - `/Users/roy/udexreal/lerobot/src/lerobot/scripts/lerobot_record.py`
> - AIRBOT SDK 文档：
>   - https://docs.airbots.online/airbot-play/sdk/api/examples.html
>   - https://docs.airbots.online/airbot-play/sdk/api/types/control-options.html
>   - https://docs.airbots.online/airbot-play/sdk/api/types/cartesian-pose.html
>   - https://docs.airbots.online/airbot-play/sdk/concepts/control-modes.html
>   - https://docs.airbots.online/airbot-play/hardware-driver/tutorials/data-collection.html
> - 创建日期：2026-04-20
> - 状态：已实施，T4 待真机确认
>
> 说明：当前仓库没有这条能力对应的已确认 `spec.md`。本文基于现有代码、LeRobot 架构和 Airbot 官方文档整理，作为可直接落地的技术方案草案。

## 1. 技术方案概述

目标是把现有 VR 手柄位姿工程、Airbot Play SDK、LeRobot 的遥操作与数据采集框架接成一条完整链路，使系统具备下面两种能力：

1. 使用 VR 手柄实时控制 Airbot Play 机械臂；
2. 在 LeRobot 中录制带相机观测、机器人状态、动作数据的数据集。

整体思路是把现有体系拆成两层：

- `Teleoperator` 层只负责接收 VR 输入，输出 LeRobot 可消费的动作；
- `Robot` 层只负责连接 Airbot SDK，读取观测并执行动作。

也就是说，`SteamMode.cpp` 不再直接“控制机械臂”，而是变成“VR 输入源”；真正的机械臂控制统一收敛到 LeRobot 的 `Robot.send_action()`。这样链路会变成：

`SteamMode.cpp -> AirbotVRTeleoperator -> LeRobot record/teleoperate loop -> AirbotPlayRobot -> Airbot SDK -> 机械臂`

这套拆法有三个直接好处：

- 架构和 LeRobot 现有设计一致，后续训练、回放、评估都更顺；
- 遥操作与采集使用同一条控制链，避免“能控制但不能录制”；
- 机械臂控制集中在一处，安全限制、控制模式切换、故障处理更容易统一。

## 2. 现状分析

### 2.1 现有 VR 工程能力

`SteamMode.cpp` 已经具备下面这些能力：

- 读取右手手柄 6DoF 位姿；
- 读取按键、扳机、握把状态；
- 用扳机控制“开始跟随 / 停止跟随”；
- 将 VR 坐标系转换到 Airbot 目标坐标系；
- 在 C++ 侧做一层安全球限幅；
- 通过 TCP 发送 `{pos, quat}` 给 Python 服务端；
- 用 B 键触发复位，用 `recenter()` 做 VR 重新居中。

这说明“VR 输入采集”和“初步跟随逻辑”已经齐了，缺的不是能力，而是 LeRobot 这一层的标准接入。

### 2.2 现有 Python 桥接脚本能力

`vr_connect_arm.py` 已经具备下面这些能力：

- 使用 `AirbotClient` 直接连接 gRPC 服务端；
- 通过 lease 获取机械臂控制权；
- 通过 TCP 接收目标位姿；
- 通过 `move_end_pose()` 下发末端位姿；
- 通过 UDP 提供当前末端位姿反馈；
- 通过特殊位置值 `[999, 999, 999]` 触发 `return_zero()`。

它的问题不在“能不能控”，而在“还没进入 LeRobot 抽象”：

- 它不是 `Teleoperator`；
- 它不是 `Robot`；
- 它的数据结构没有对齐 LeRobot 的动作和观测特征；
- 它的 TCP 协议过于薄，只传 `pos/quat`，缺按钮、扳机、夹爪、事件字段；
- 它把网络桥接、控制逻辑、协议约定都揉在一个脚本里，不利于复用。

### 2.3 LeRobot 现有能力

LeRobot 现有链路已经很完整：

- `Teleoperator.get_action()` 负责产出动作；
- `Robot.get_observation()` 负责产出观测；
- `Robot.send_action()` 负责执行动作；
- `lerobot-teleoperate` 负责实时遥操作；
- `lerobot-record` 或定制 `record.py` 负责采集数据。

仓库里还有两类很有参考价值的例子：

- `phone_to_so100`：输入是 6DoF 姿态，经过 processor 映射到末端动作；
- `so100_to_so100_EE`：输入是主臂末端，输出是从臂末端，再经过 IK/安全处理。

这说明 VR 遥操作并不是异类，本质上和 `phone` 那条链很像，只是输入源从手机换成了 VR 手柄。

## 3. 技术决策

### 3.1 分层方案

#### 方案 A：沿用现有桥接结构

链路：

`SteamMode.cpp -> vr_connect_arm.py -> Airbot SDK`

优点：

- 改动最少；
- 最快能动起来。

缺点：

- 不进入 LeRobot 的 `Teleoperator` / `Robot` 抽象；
- 不能自然接入 `lerobot-teleoperate`、`lerobot-record`；
- 数据采集仍然需要另写一套逻辑；
- 后续训练和回放链路会割裂。

#### 方案 B：VR 作为 Teleoperator，Airbot SDK 作为 Robot

链路：

`SteamMode.cpp -> AirbotVRTeleoperator -> AirbotPlayRobot -> Airbot SDK`

优点：

- 完全对齐 LeRobot 的标准结构；
- 遥操作和录制走同一条链；
- 便于测试、调参和复用；
- 后续可继续扩展到评估、策略接管、HIL 等场景。

缺点：

- 前期接入工作更细；
- 需要同时设计 Teleoperator、Robot、协议和录制脚本。

#### 最终选择

选择方案 B。

这是唯一能真正“打通 LeRobot VR 遥操作和数据采集链路”的方案。方案 A 只适合临时演示，不适合作为正式集成路径。

### 3.2 代码落地位置

#### 方案 A：直接改 LeRobot 主仓

优点：

- 最省事；
- 配置和脚本接入简单；
- 不需要额外打包。

缺点：

- 会把项目私有硬件接入写进主树；
- 后续同步上游会更麻烦。

#### 方案 B：做成第三方 LeRobot 插件包

优点：

- 和上游主仓解耦；
- 结构更干净；
- 可复用到别的仓库。

缺点：

- 打包、安装、版本管理更复杂；
- 当前阶段联调成本更高。

#### 最终选择

第一阶段直接在当前仓库落地，待接口稳定后再抽成第三方插件。

原因：

- 现在的主要目标是先把链路打通；
- 当前仓库已经在放这套 VR 和 Airbot 相关代码；
- LeRobot 已经支持后续插件化，先跑通再拆更稳。

### 3.3 Teleoperator 的职责边界

最终约定：

- `AirbotVRTeleoperator` 只接 VR 消息，不直接调用 Airbot SDK；
- 它对外暴露 LeRobot 动作字典；
- 它内部负责输入缓存、协议解析、过期判断、事件提取；
- 它不负责真的“执行动作”。

### 3.4 Robot 的职责边界

最终约定：

- `AirbotPlayRobot` 负责连接 `AirbotClient`；
- 负责 lease 生命周期；
- 负责控制模式切换；
- 负责 `get_observation()`；
- 负责把 LeRobot 动作转换成 `CartesianPose + ArmControlOptions`；
- 负责安全保持、异常恢复、断开清理。

### 3.5 控制模式选择

#### 方案 A：以 `direct_control` 作为默认模式

优点：

- 直控链路短；
- 理论延迟更低。

缺点：

- 根据 Airbot 官方控制模式说明，`move_end_pose` 不作为 `direct_control` 的主支持路径；
- 当前场景是 VR 高频末端跟随，不是关节级低层直控；
- 第一阶段走这条路，联调风险高。

#### 方案 B：以 `servo_control` 作为默认模式

优点：

- 官方说明里明确标注 `servo_control` 适合遥操作；
- `move_end_pose` 在该模式下可用；
- 官方说明中还写明 `move_end_pose` 可同时控制机械臂和末端执行器；
- 更贴合当前 VR 末端跟随场景。

缺点：

- 需要连续稳定地下发命令，对消息时序更敏感。

#### 最终选择

第一阶段默认使用 `servo_control`。

补充约定：

- `direct_control` 不进入第一阶段主路径；
- `direct_control` 只作为后备实验项保留在配置层；
- 联调时优先验证 `servo_control + move_end_pose` 的稳定性。

### 3.6 动作表示方案

#### 方案 A：LeRobot 内部全部使用四元数

优点：

- 和 Airbot SDK 接口最贴近；
- 少一次姿态转换。

缺点：

- LeRobot 现有末端动作和末端观测大多使用 `pos + rotvec`；
- 数据集风格和现有例子不一致；
- 后续若接处理器、模型或别的机器人，复用性偏差。

#### 方案 B：LeRobot 内部统一使用 `pos + rotvec`

优点：

- 和 `phone_to_so100`、`so100_to_so100_EE` 例子一致；
- 更贴近 LeRobot 现有末端动作表达；
- 后续做数据分析、建模更统一。

缺点：

- 在 `Robot.send_action()` 边界处要做一次 `rotvec -> quaternion` 转换。

#### 最终选择

选择方案 B。

因此约定：

- Teleoperator 输出：`ee.x ee.y ee.z ee.wx ee.wy ee.wz ee.gripper_pos`
- Robot 接收同样字段；
- Robot 内部在调用 Airbot SDK 前，把 `rotvec` 转成四元数 `(x, y, z, w)`。

### 3.7 是否继续保留 C++ 侧坐标变换

#### 方案 A：继续在 `SteamMode.cpp` 中完成 VR 到 Airbot 坐标变换

优点：

- 复用现有逻辑最多；
- 风险最低；
- 联调最短。

缺点：

- 坐标约定散落在 C++；
- Python 侧可观察性一般；
- 以后换设备时复用差。

#### 方案 B：C++ 只发原始 VR 位姿，Python 侧处理

优点：

- LeRobot 侧更统一；
- 更容易做日志、可视化和调参；
- 更利于后续抽象出通用 VR Teleoperator。

缺点：

- 需要重写现有变换和跟随逻辑；
- 初期联调成本高。

#### 最终选择

第一阶段选择方案 A，保留现有 C++ 坐标变换和跟随逻辑。

原因：

- 这套变换逻辑已经在实机链路里跑过；
- 先把 LeRobot 接进来更重要；
- 待链路稳定后，再考虑把变换下沉到 Python。

这意味着第一阶段的 `AirbotVRTeleoperator` 接收到的不是“原始 VR 位姿”，而是“已经对齐 Airbot 目标坐标系的末端目标”。

迁移路线约定：

- 第一阶段保留 C++ 侧变换；
- 第二阶段先由 Python processor 复算同一份坐标变换，并保留一段 C++ / Python 双写校验窗口；
- 一致性稳定后，再考虑把 C++ 端变换下沉为“只发原始 VR 数据”。

### 3.8 协议设计方案

#### 现状

当前 TCP 消息只有：

```json
{"pos":[x,y,z],"quat":[w,x,y,z]}
```

问题：

- 没有消息边界；
- 没有版本号；
- 没有时间戳；
- 没有跟踪状态；
- 没有按钮、扳机、握把；
- 没有显式事件字段。

#### 最终选择

改成单行 JSON 协议，使用 `\n` 作为帧边界，字段如下：

```json
{
  "version": 1,
  "timestamp_ms": 1713599999999,
  "frame": "airbot_base",
  "tracking_ok": true,
  "following": true,
  "pose": {
    "pos": [0.401, 0.012, 0.298],
    "quat_wxyz": [0.707, 0.0, 0.707, 0.0]
  },
  "analog": {
    "trigger": 0.91,
    "grip": 0.10,
    "joystick_x": 0.02,
    "joystick_y": -0.04
  },
  "buttons": {
    "trigger": true,
    "grip": false,
    "a": false,
    "b": false,
    "menu": false,
    "joystick_press": false
  },
  "events": {
    "reset_robot": false,
    "recenter_vr": false
  }
}
```

采用这一版协议的理由：

- Python 侧解析简单；
- 便于扩字段；
- 能兼容调试工具抓包；
- 可以显式处理粘包和拆包。

### 3.9 录制链路选择

#### 方案 A：直接使用通用 `lerobot-record`

优点：

- 命令行接入最顺；
- 和 LeRobot 现有用法一致。

缺点：

- 只能很好处理“teleop 动作和 robot.action_features 完全一致”的场景；
- 不擅长处理 VR 自带的事件流，比如 `reset_robot`、`recenter_vr`、自定义 episode 控制。

#### 方案 B：先做一份 Airbot VR 专用 `record.py`

优点：

- 可以显式处理 B 键复位、设备重连、消息过期、状态播报；
- 更适合第一阶段联调；
- 后续成熟后再回收进通用脚本。

缺点：

- 多一份专用脚本需要维护。

#### 最终选择

第一阶段采用方案 B。

同时保持 `AirbotVRTeleoperator.action_features` 和 `AirbotPlayRobot.action_features` 尽量对齐，为后续并入通用 `lerobot-record` 留口子。

### 3.10 数据集字段策略

#### 方案 A：只记录机器人实际动作

优点：

- schema 简单；
- 训练主标签最干净。

缺点：

- 不利于排查 VR 输入和机器人执行之间的偏差；
- 后续难以分析 tracking 丢失、坐标映射误差和操作者意图偏差。

#### 方案 B：同时记录机器人实际动作和精简版原始 VR 输入

优点：

- 训练仍可用机器人实际动作作为主标签；
- 同时保留调试和分析所需的 VR 输入证据；
- 更利于后续做对齐分析和输入质量评估。

缺点：

- 数据集字段稍多；
- 需要提前约束哪些 VR 字段进入数据集。

#### 最终选择

第一阶段选择方案 B。

记录策略约定：

- `action` 主字段记录机器人实际执行动作；
- 额外记录一组精简版 VR 辅助字段；
- 不把整包原始 JSON 直接塞进数据集；
- 第一阶段建议保留的 VR 辅助字段包括：
  - `teleop.vr.pos.x/y/z`
  - `teleop.vr.quat.w/x/y/z`
  - `teleop.vr.trigger`
  - `teleop.vr.grip`
  - `teleop.vr.tracking_ok`
- 第一阶段不再额外加入 `joystick_x/y`、`menu`、tracking 失败原因等字段；
- 若联调时确实要看这些信息，先写调试日志，不进入正式数据集 schema。

### 3.11 VR 按键语义

第一阶段先收紧按键职责，避免把安全动作和数据流程动作混在一起。

最终约定：

- `Trigger`：开始跟随 / 停止跟随
- `B`：机器人复位 / 回零，并触发 VR 重新居中
- `B` 不承担 episode 保存、结束或重录语义
- 第一阶段 episode 流程控制仍以键盘或脚本外层逻辑为主

第二阶段预留扩展：

- `A` 或 `JoystickPress`：结束当前 episode
- `Y`：重录当前 episode
- `Menu`：停止整轮采集

这样拆分的好处是：

- 误触 `B` 不会直接污染数据流程；
- 复位语义更单纯；
- 现场联调更容易排查问题。

## 4. 模块设计

### 4.1 Teleoperator 模块

建议新增目录：

- `src/lerobot/teleoperators/airbot_vr/__init__.py`
- `src/lerobot/teleoperators/airbot_vr/config_airbot_vr.py`
- `src/lerobot/teleoperators/airbot_vr/airbot_vr.py`

#### 配置项建议

`AirbotVRTeleoperatorConfig` 建议包含：

- `host`: 监听地址，默认 `0.0.0.0`
- `port`: 监听端口，默认不要继续复用 `5005`，建议单独配置
- `id`: 设备标识
- `packet_timeout_s`: 包过期时间
- `startup_wait_s`: 启动后等待首包的最长时间
- `default_gripper_pos`: 默认夹爪开合值
- `max_position_jump_m`: Teleoperator 侧允许的单帧位置跳变
- `accept_tracking_lost`: 丢跟踪时是否保留最后动作
- `calibration_dir`: 兼容 LeRobot 基类接口

#### 运行逻辑

- `connect()`
  - 启动 TCP server；
  - 启动后台接收线程；
  - 等待首个有效包；
  - 初始化内部缓存。
- `get_action()`
  - 读取最近一帧消息；
  - 检查是否过期；
  - `quat_wxyz -> quat_xyzw -> rotvec`；
  - 输出 LeRobot 兼容动作字典；
  - 若包过期或 tracking 失效，返回“最后一帧安全动作”。
- `get_teleop_events()`
  - 输出 `reset_robot`、`recenter_vr`、`rerecord_episode`、`terminate_episode` 等扩展事件；
  - 第一阶段至少要支持 `reset_robot` 与 `recenter_vr`。
- `disconnect()`
  - 停接收线程；
  - 关闭监听 socket；
  - 清理缓存。

#### `action_features` 建议

```python
{
  "ee.x": float,
  "ee.y": float,
  "ee.z": float,
  "ee.wx": float,
  "ee.wy": float,
  "ee.wz": float,
  "ee.gripper_pos": float,
}
```

这样做的好处是：

- 跟现有 EE 控制例子一致；
- 后续容易做数据集特征对齐；
- 如果 `Robot.action_features` 也保持一致，通用脚本更容易兼容。

### 4.2 Robot 模块

建议新增目录：

- `src/lerobot/robots/airbot_play/__init__.py`
- `src/lerobot/robots/airbot_play/config_airbot_play.py`
- `src/lerobot/robots/airbot_play/airbot_play.py`

#### 配置项建议

`AirbotPlayRobotConfig` 建议包含：

- `host`: Airbot gRPC 地址
- `port`: Airbot gRPC 端口
- `arm_dof`: 默认 `6`
- `id`: 设备标识
- `controller_mode`: 默认 `servo_control`，`direct_control` 仅作后备实验项
- `lease_ms`
- `renew_period_s`
- `timeout_ms`
- `control_rate_hz`
- `set_arm_speed`: 可选
- `set_eef_speed`: 可选
- `default_eef_eff`
- `default_arm_eff`
- `blocking`
- `cameras`
- `enable_gripper`
- `gripper_control_mode`: 默认 `embedded_with_move_end_pose`，异常时切到 `separate_servo`
- `disconnect_return_zero`

#### `connect()` 逻辑

- 初始化 `AirbotClient`；
- 获取控制权；
- 切换控制模式；
- 设置速度参数；
- 初始化相机；
- 读取一帧当前末端位姿，作为保持位姿的起点。

#### `observation_features` 建议

最少包括：

- `ee.x`
- `ee.y`
- `ee.z`
- `ee.wx`
- `ee.wy`
- `ee.wz`
- `ee.gripper_pos`

建议同时补充：

- `joint_1.pos ... joint_6.pos`
- `joint_1.vel ... joint_6.vel`（如果 SDK 提供且读取稳定）
- `observation.images.<camera_name>`

#### `action_features` 建议

与 Teleoperator 保持一致：

```python
{
  "ee.x": float,
  "ee.y": float,
  "ee.z": float,
  "ee.wx": float,
  "ee.wy": float,
  "ee.wz": float,
  "ee.gripper_pos": float,
}
```

#### `send_action()` 逻辑

1. 校验动作字段完整性；
2. 对位置做边界和单步跳变限制；
3. `rotvec -> quaternion(x, y, z, w)`；
4. 构造 `CartesianPose`；
5. 构造 `ArmControlOptions`；
6. 优先把夹爪目标写到 `options.eef_pos`，与末端位姿一起下发；
7. 调用 `move_end_pose()`；
8. 若随 `move_end_pose()` 一起控制夹爪不稳定，再退化为独立连续夹爪伺服接口，不使用规划式单次夹爪命令作为主路径。

### 4.3 数据协议与线程模型

#### Teleoperator 线程模型

- 主线程：LeRobot 录制循环调用 `get_action()`
- 后台线程：监听 TCP，按行解析 JSON，更新最新状态

内部建议维护：

- `latest_packet`
- `last_valid_action`
- `last_packet_time`
- `pending_events`
- `connection_state`

#### 过期策略

- 若首包未到：`connect()` 阶段阻塞等待，超时报错；
- 若运行中断流：
  - 短时断流：继续输出 `last_valid_action`
  - 长时断流：记录告警，并拒绝大幅更新
- 若 tracking 丢失：
  - 保持最后一帧动作，不发送跳变动作

### 4.4 录制脚本

建议新增：

- `examples/airbot_play_vr/teleoperate.py`
- `examples/airbot_play_vr/record.py`

第一阶段不建议直接硬塞进通用 `lerobot-record`，原因是：

- 需要处理 `reset_robot`；
- 需要处理 `recenter_vr`；
- 需要做 Airbot 专属状态播报；
- 需要明确控制 episode 保存、重录和复位节奏。

专用 `record.py` 建议：

- 复用 `record_loop()` 主体；
- 但在外层加一层 Airbot VR 事件处理；
- 当收到 `reset_robot` 时：
  - 暂停当前 episode 的动作发送；
  - 调用 `robot.return_zero()` 或封装的 `robot.reset_to_home()`；
  - 清空 Teleoperator 的 pending event；
  - 等待操作者重新就位。
- 写入数据集时：
  - 以机器人实际动作作为主 `action`
  - 额外写入精简版 VR 辅助字段，便于排查和分析

## 5. 任务拆解

### 5.1 共享与基础设施

- [x] **TODO-G1: 明确动作与观测字段约定**
  - **描述**：冻结 `AirbotVRTeleoperator` 和 `AirbotPlayRobot` 之间的统一动作/观测字典，确定是否使用 `rotvec`、如何表示夹爪。
  - **涉及文件**：
    - `udexreal_dc/airbot_vr_lerobot_technical_plan.md`
    - `src/lerobot/robots/airbot_play/config_airbot_play.py`
    - `src/lerobot/teleoperators/airbot_vr/config_airbot_vr.py`
  - **依赖**：无
  - **验收标准**：动作字段和观测字段清单固定，可直接用于 `action_features` / `observation_features`

- [x] **TODO-G2: 明确 TCP 协议并替换现有裸 JSON**
  - **描述**：把当前只有 `pos/quat` 的消息改成带版本号、时间戳、按键、事件的单行 JSON 协议。
  - **涉及文件**：
    - `udexreal_dc/assistant4/embodied/SteamMode/SteamMode.cpp`
    - `udexreal_dc/assistant4/embodied/SteamMode/arm_comm.cpp`
    - `udexreal_dc/assistant4/embodied/SteamMode/arm_comm.h`
  - **依赖**：TODO-G1
  - **验收标准**：协议文档固定，支持按行解析，抓包能稳定读到完整消息

- [x] **TODO-G3: 补 Airbot 依赖接入策略**
  - **描述**：确定 `arm_sdk` 的导入保护、可选依赖声明、缺依赖时报错方式。
  - **涉及文件**：
    - `pyproject.toml`
    - `src/lerobot/utils/import_utils.py`
  - **依赖**：无
  - **验收标准**：未安装 `arm_sdk` 时，报错清楚；安装后可正常导入

### 5.2 VR 输入层

- [x] **TODO-S1: 调整 SteamMode 发送内容**
  - **描述**：保留现有跟随与坐标变换逻辑，但把发送端从“只发 pose”改成“发 pose + analog + buttons + events”。
  - **涉及模块**：VR 输入运行时
  - **涉及文件**：
    - `udexreal_dc/assistant4/embodied/SteamMode/SteamMode.cpp`
    - `udexreal_dc/assistant4/embodied/SteamMode/arm_comm.cpp`
  - **依赖**：TODO-G2
  - **验收标准**：Python 侧能稳定收到完整帧；B 键、扳机、握把状态可见

- [x] **TODO-S2: 实现 AirbotVRTeleoperator 配置类**
  - **描述**：创建 Teleoperator 配置类，注册到 draccus，支持命令行和脚本实例化。
  - **涉及模块**：Teleoperator
  - **涉及文件**：
    - `src/lerobot/teleoperators/airbot_vr/config_airbot_vr.py`
    - `src/lerobot/teleoperators/airbot_vr/__init__.py`
  - **依赖**：TODO-G1
  - **验收标准**：可通过配置实例化 Teleoperator

- [x] **TODO-S3: 实现 AirbotVRTeleoperator 运行时**
  - **描述**：实现后台 TCP 接收、缓存、包解析、过期判断、动作输出。
  - **涉及模块**：Teleoperator
  - **涉及文件**：
    - `src/lerobot/teleoperators/airbot_vr/airbot_vr.py`
  - **依赖**：TODO-S2
  - **验收标准**：`get_action()` 能稳定返回 LeRobot 动作字典；断流时不抖动

- [x] **TODO-S4: 实现 VR 事件接口**
  - **描述**：实现 `get_teleop_events()`，第一阶段至少支持 `reset_robot` 与 `recenter_vr`，episode 控制事件先只预留接口，不进入主流程。
  - **涉及模块**：Teleoperator
  - **涉及文件**：
    - `src/lerobot/teleoperators/airbot_vr/airbot_vr.py`
  - **依赖**：TODO-S3
  - **验收标准**：收到 B 键或其它事件时，Python 侧能拿到一次性事件标记

### 5.3 Airbot 机器人层

- [x] **TODO-S5: 实现 AirbotPlayRobot 配置类**
  - **描述**：创建 Robot 配置类，默认走 `servo_control`，并支持 gRPC 地址、速度、夹爪和相机参数。
  - **涉及模块**：Robot
  - **涉及文件**：
    - `src/lerobot/robots/airbot_play/config_airbot_play.py`
    - `src/lerobot/robots/airbot_play/__init__.py`
  - **依赖**：TODO-G1
  - **验收标准**：可通过配置实例化 Robot

- [x] **TODO-S6: 实现 AirbotPlayRobot.connect / disconnect**
  - **描述**：实现 client 生命周期、lease 获取与释放、控制模式切换、相机连接。
  - **涉及模块**：Robot
  - **涉及文件**：
    - `src/lerobot/robots/airbot_play/airbot_play.py`
  - **依赖**：TODO-S5、TODO-G3
  - **验收标准**：连接和断开流程稳定，异常中断后能清理

- [x] **TODO-S7: 实现 AirbotPlayRobot.get_observation**
  - **描述**：读取末端位姿、夹爪状态、关节状态和图像，输出标准观测字典。
  - **涉及模块**：Robot
  - **涉及文件**：
    - `src/lerobot/robots/airbot_play/airbot_play.py`
  - **依赖**：TODO-S6
  - **验收标准**：观测字段完整，连续读取稳定，不因个别字段失败导致整体崩

- [x] **TODO-S8: 实现 AirbotPlayRobot.send_action**
  - **描述**：把 `ee.x/y/z/wx/wy/wz/gripper_pos` 转成 `CartesianPose + ArmControlOptions`，默认走“位姿+夹爪一起下发”，并加一层安全约束。
  - **涉及模块**：Robot
  - **涉及文件**：
    - `src/lerobot/robots/airbot_play/airbot_play.py`
  - **依赖**：TODO-S6、TODO-G1
  - **验收标准**：实机能跟随；位置和姿态更新稳定；夹爪动作可用

- [x] **TODO-S9: 实现 Airbot 专用复位接口**
  - **描述**：封装 `return_zero()` 或统一的 `reset_to_home()`，供录制脚本调用。
  - **涉及模块**：Robot
  - **涉及文件**：
    - `src/lerobot/robots/airbot_play/airbot_play.py`
  - **依赖**：TODO-S8
  - **验收标准**：收到复位请求后能回到安全初始位姿

### 5.4 遥操作与采集脚本

- [x] **TODO-C1: 新增 Airbot VR 遥操作示例**
  - **描述**：实现最小 `teleoperate.py`，用于验证 VR 输入到机械臂动作的链路。
  - **涉及模块**：示例脚本
  - **涉及文件**：
    - `examples/airbot_play_vr/teleoperate.py`
  - **依赖**：TODO-S3、TODO-S8
  - **验收标准**：命令行运行后，VR 手柄可实时控制 Airbot

- [x] **TODO-C2: 新增 Airbot VR 录制脚本**
  - **描述**：实现专用 `record.py`，处理 VR 事件、episode 保存、复位与数据集落盘。
  - **涉及模块**：示例脚本
  - **涉及文件**：
    - `examples/airbot_play_vr/record.py`
  - **依赖**：TODO-S4、TODO-S7、TODO-S9
  - **验收标准**：可连续录制多轮 episode，图像、观测、动作都能写入数据集

### 5.5 测试与验证

- [x] **TODO-T1: 补 Teleoperator 单元测试**
  - **描述**：覆盖协议解析、断流、过期、姿态转换、事件消费。
  - **涉及文件**：
    - `tests/teleoperators/test_airbot_vr.py`
  - **依赖**：TODO-S4
  - **验收标准**：核心输入逻辑都有自动化覆盖

- [x] **TODO-T2: 补 Robot 单元测试**
  - **描述**：用 mock client 覆盖连接、观测、动作转换、异常处理。
  - **涉及文件**：
    - `tests/robots/test_airbot_play.py`
  - **依赖**：TODO-S9
  - **验收标准**：不连真机也能验证主要行为

- [x] **TODO-T3: 补端到端联调清单**
  - **描述**：整理真机联调步骤、环境依赖和验收项，供后续反复回归。
  - **涉及文件**：
    - `examples/airbot_play_vr/README.md`
  - **依赖**：TODO-C2
  - **验收标准**：新机器按文档可复现联调

- [ ] **TODO-T4: 真机确认 `options.eef_pos` 语义**
  - **描述**：选 5 档夹爪目标值，逐档下发并回读，确认 `ArmControlOptions.eef_pos` 的单位、方向、边界和生效条件。
  - **涉及文件**：
    - `examples/airbot_play_vr/README.md`
    - `udexreal_dc/airbot_vr_lerobot_technical_plan.md`
  - **依赖**：TODO-S8
  - **验收标准**：确认它能否按“夹爪开口宽度，单位米”使用；若不一致，明确回退到独立连续夹爪伺服接口

## 6. 依赖关系与执行顺序

建议顺序：

1. `TODO-G1 -> TODO-G2 -> TODO-S1`
2. `TODO-G1 -> TODO-S2 -> TODO-S3 -> TODO-S4`
3. `TODO-G1 -> TODO-G3 -> TODO-S5 -> TODO-S6 -> TODO-S7 -> TODO-S8 -> TODO-S9`
4. `TODO-S3 + TODO-S8 -> TODO-C1`
5. `TODO-S4 + TODO-S7 + TODO-S9 -> TODO-C2`
6. `TODO-S4 -> TODO-T1`
7. `TODO-S9 -> TODO-T2`
8. `TODO-S8 -> TODO-T4`
9. `TODO-C2 -> TODO-T3`

可并行部分：

- `TODO-S1` 和 `TODO-S2` 可并行
- `TODO-T1` 和 `TODO-T2` 可并行
- `TODO-C1` 可先于 `TODO-C2` 完成

ASCII 图：

```text
G1 -> G2 -> S1
G1 -> S2 -> S3 -> S4
G1 -> G3 -> S5 -> S6 -> S7 -> S8 -> S9
S3 + S8 -> C1
S4 + S7 + S9 -> C2
S4 -> T1
S9 -> T2
S8 -> T4
C2 -> T3
```

## 7. 测试标准

### 7.1 单元测试标准

#### Teleoperator

- 能正确解析单行 JSON 协议
- 能处理粘包、拆包、空包、非法 JSON
- 能把 `quat_wxyz` 正确转成 `rotvec`
- 首包未到时会超时
- 包过期时保持最后一帧动作
- `reset_robot` 事件只消费一次
- tracking 失效时不会输出突变动作

#### Robot

- `connect()` 会正确初始化 client、lease、控制模式
- `disconnect()` 能释放控制权并关闭连接
- `get_observation()` 能把 Airbot 返回值转成约定字段
- `send_action()` 能把 `rotvec` 正确转成四元数
- `send_action()` 会把 `gripper_pos` 写入 `options.eef_pos`
- 动作字段缺失时会报清楚的错误
- SDK 调用失败时不会让内部状态错乱

### 7.2 集成 / 场景验证标准

#### 场景 1：最小遥操作联调

- **前置条件**：
  - Airbot gRPC 服务端已启动
  - SteamMode 已启动
  - 右手手柄已连上
- **操作步骤**：
  - 启动 `examples/airbot_play_vr/teleoperate.py`
  - 扣下扳机，移动手柄
  - 松开扳机，观察机械臂保持
- **期望结果**：
  - 扣下扳机后机械臂跟随
  - 松开扳机后机械臂保持最后姿态
  - 无明显跳变或抖动

#### 场景 2：夹爪联调

- **操作步骤**：
  - 改变握把或约定的夹爪输入
  - 观察夹爪位置变化
- **期望结果**：
  - 夹爪动作方向与输入一致
  - 夹爪不会越限
  - 手臂与夹爪同步时无明显相位差

#### 场景 3：录制单轮 episode

- **操作步骤**：
  - 启动 `examples/airbot_play_vr/record.py`
  - 完成一轮演示
  - 通过键盘或约定流程保存 episode
- **期望结果**：
  - 数据集目录生成成功
  - 图像、观测、动作数量一致
  - 动作字段包含末端位姿和夹爪
  - 数据集额外包含精简版 VR 辅助字段

#### 场景 4：复位流程

- **操作步骤**：
  - 录制中触发 `reset_robot`
- **期望结果**：
  - 机械臂安全回零或回初始位
  - VR 侧不会继续发旧目标导致抢控制
  - 复位完成后可继续下一轮

#### 场景 5：异常断连

- **操作步骤**：
  - 遥操作中断开 SteamMode 或网络
- **期望结果**：
  - Teleoperator 检测到断流
  - 机器人保持最后安全姿态
  - 不发生大幅突变

## 8. 风险与缓解

| 风险 | 影响 | 缓解措施 |
|------|------|---------|
| Airbot 文档对 `move_end_pose` 的控制模式说明前后不一致 | 选错控制模式会导致实机不响应或表现异常 | 第一阶段固定 `servo_control`；`direct_control` 仅保留为后备实验选项 |
| VR 坐标系与 Airbot 基坐标对不上 | 机械臂方向反、姿态乱跳 | 第一阶段保留 C++ 端现有变换；补抓包日志和姿态打印，逐项核对 |
| 当前 TCP 裸 JSON 存在粘包问题 | Teleoperator 解析不稳定 | 改成单行 JSON，按换行切帧 |
| 断流或 tracking 丢失时动作突变 | 存在安全风险 | Teleoperator 保持最后一帧安全动作，Robot 再做一层单步限幅 |
| 夹爪控制方式不明确 | 夹爪动作失败或跟主臂动作互相影响 | 先用 `options.eef_pos` 随 `move_end_pose()` 一起控制；若不稳定，再退化为独立连续夹爪伺服接口 |
| 通用 `lerobot-record` 不好处理 VR 事件 | 录制流程卡住 | 第一阶段使用专用 `record.py` |
| 相机读取拖慢控制频率 | 遥操作卡顿，录制掉帧 | 控制和图像读取分开看待，先以低分辨率联调，再调回目标配置 |

## 9. 已定项与联调验证项

### 9.1 已定项

1. 第一阶段数据集只保留精简版 VR 辅助字段，不再加入 `joystick_x/y`、`menu`、tracking 失败原因等额外字段。
2. 第二阶段若把坐标变换迁到 Python，必须先保留一段 C++ / Python 双写校验窗口，再决定是否切走 C++ 侧变换。

### 9.2 联调验证项

1. `ArmControlOptions.eef_pos` 在当前 `arm_sdk` 版本中的单位、边界和生效条件，需要用真机确认。
2. 验证通过前，方案默认按“夹爪开口宽度，单位米”处理，并保留退回独立连续夹爪伺服接口的开关。

## 10. 推荐开工顺序

如果目标是最短时间跑通，建议按下面的顺序推进：

1. 先冻结动作/观测字段；
2. 改 TCP 协议；
3. 先做 `AirbotVRTeleoperator`，确认 Python 能稳定收到 VR 数据；
4. 再做 `AirbotPlayRobot`，先单独验证 `get_observation()` 和 `send_action()`；
5. 先用真机确认 `options.eef_pos` 的实际语义，再定是否走夹爪回退路径；
6. 先跑 `teleoperate.py`，确认实时控制稳定；
7. 最后再补 `record.py` 和测试。

这样推进，能把问题拆成三段：

- VR 输入是否稳定；
- Airbot SDK 控制是否稳定；
- LeRobot 录制是否稳定。

每段都能单独验，不会所有问题混在一起。
