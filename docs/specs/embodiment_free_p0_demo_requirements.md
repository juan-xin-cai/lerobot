# P0 无本体采集完整 Demo 需求文档

> 范围：手套 + SteamVR Tracker + 腕部鱼眼摄像头，基于 LeRobot 完成最小可用数据采集闭环。  
> 目标：形成可录制、可回放检查、可扩展到后处理/重定向/训练的数据基线。  
> 状态：需求定义；腕部鱼眼相机链路已完成最小验证。

## 1. Demo 目标

本 demo 要在没有机器人本体的情况下，完成一条完整 episode 的多模态采集：

- 腕部鱼眼摄像头采集第一视角图像。
- 手套采集手指/手部动作。
- SteamVR Tracker 采集腕部 6DoF 位姿。
- LeRobot 将三路数据写入统一 `LeRobotDataset`。
- 采集时保留主机接收时间戳和 seq，用于 P0 级同步检查。
- Rerun 可实时显示图像和低维曲线。

## 2. 非目标

P0 不要求：

- 机器人真机控制。
- 硬件级同步或设备级硬时间戳。
- 完整 retargeting。
- 完整标注系统。
- 自动质量评分闭环。
- 数据库检索和对象存储。

但 P0 必须为这些能力预留字段和目录结构。

## 3. 系统组成

```text
腕部鱼眼摄像头 (/dev/video0)
        ↓
CaptureRigRobot
        ↓ observation.images.wrist_fisheye

手套
        ↓
GloveTeleoperator
        ↓ action.hand_joints / glove timestamp / seq

SteamVR Tracker
        ↓
SteamVRTrackerTeleoperator
        ↓ action.left_wrist_tracker / action.right_wrist_tracker / tracker timestamp / seq / valid

CompositeTeleoperator
        ↓
lerobot-record
        ↓
LeRobotDataset + platform metadata
```

## 4. 模块需求

### 4.1 腕部鱼眼摄像头

当前已验证：

```yaml
type: opencv
index_or_path: /dev/video0
width: 1920
height: 1536
fps: 30
fourcc: UYVY
```

输出 LeRobot feature：

```text
observation.images.wrist_fisheye
dtype: video
shape: [1536, 1920, 3]
```

P0 需要补充：

```text
observation.state.camera_receive_time
observation.state.camera_seq
```

说明：

- `camera_receive_time` 使用 `time.perf_counter()`，不是硬件时间。
- `camera_seq` 每次相机读线程获得新帧时递增。

### 4.2 手套

手套作为 teleoperator 输入源之一。

输出建议：

```text
action.glove.left_thumb_third_joint_pitch
...
action.glove.left_imu_quaternion_z
action.glove.right_thumb_third_joint_pitch
...
action.glove.right_imu_quaternion_z
action.glove_receive_time
action.glove_seq
action.glove_valid
```

最低字段定义：

| 字段 | 类型 | shape | 说明 |
|------|------|-------|------|
| `action.glove.left_*` | float32 | `[28]` | 左手 28 个 Udhand 参数，字段名使用 `l0-l27` 注释含义 |
| `action.glove.right_*` | float32 | `[28]` | 右手 28 个 Udhand 参数，字段名使用 `r0-r27` 注释含义 |
| `action.glove_receive_time` | float32 | `[1]` | 主机收到该样本的 `perf_counter` 时间 |
| `action.glove_seq` | int64/float32 | `[1]` | 手套读线程样本序号 |
| `action.glove_valid` | bool/float32 | `[1]` | 当前样本是否有效 |

P0 UDP 协议：

- 采集机作为 UDP server 监听手套 JSON。
- JSON 根字段包含 `Udhand.Parameter`。
- `Parameter` 是 `{ "Name": ..., "Value": ... }` 数组。
- P0 只从原始 JSON 读取 `l0-l27` 和 `r0-r27`；不记录 `Bones`、标定状态、摇杆和按键。
- 落库字段不使用 `l0/r0` 编号，而使用发送端每个 KV 注释对应的语义名。

P0 行为要求：

- 手套读线程独立运行，不阻塞 record loop。
- record loop 每帧取最近一条手套样本。
- 若超过阈值未更新，`glove_valid=false`，并写入质量 flag。
- 如果手套本身没有设备时间戳，只记录主机接收时间。

建议实现路径：

```text
src/lerobot/teleoperators/<glove_name>/
  config_<glove_name>.py
  <glove_name>.py
```

当前 P0 使用 `udp_glove` 接收 Udhand JSON，并加入 receive time / seq / valid。

### 4.3 SteamVR Tracker

Tracker 作为 wrist 位姿输入源。当前 P0 对齐方式：SteamVR/OpenVR 不在采集机上直接接入，
而是由外部 PC 通过 TCP 连接把 tracker 字符串推送到采集机。

输出建议：

```text
action.left_wrist_tracker.x
action.left_wrist_tracker.y
action.left_wrist_tracker.z
action.left_wrist_tracker.rot_x
action.left_wrist_tracker.rot_y
action.left_wrist_tracker.rot_z
action.right_wrist_tracker.x
action.right_wrist_tracker.y
action.right_wrist_tracker.z
action.right_wrist_tracker.rot_x
action.right_wrist_tracker.rot_y
action.right_wrist_tracker.rot_z
action.tracker_source_time
action.tracker_receive_time
action.tracker_seq
action.tracker_valid
```

TCP 协议格式示例：

```text
2026-04-02 17:39:32:050;LG=F;RG=F;LT=F;RT=F;LPos: (-0.004536, 1.138480, 0.407648) LRot: (12.439250, 69.856450, 299.125500);RPos: (0.159430, 1.139674, 0.369927) RRot: (12.275980, 310.135700, 61.552390);X=F;A=F;Y=F;B=F
```

P0 记录 `LPos` / `RPos` 作为左右腕部 tracker 位置，记录 `LRot` / `RRot` 作为外部 PC
发送的原始旋转三元组。字符串开头时间解析为当天秒数 `tracker_source_time`，采集机收到样本时记录
`tracker_receive_time`。

```text
frame_id: steamvr_standing
unit: meter
```

P0 行为要求：

- 能以 TCP server 方式接收外部 PC 推送的 SteamVR tracker 字符串。
- 能解析并记录 `LPos`、`RPos`、`LRot`、`RRot`。
- `tracker_valid=false` 时仍写入占位数据，并写质量 flag。
- 记录主机接收时间 `tracker_receive_time`。
- 记录 `tracker_seq`，用于检查数据是否长时间不更新。

建议实现路径：

```text
src/lerobot/teleoperators/steamvr_tracker/
  config_steamvr_tracker.py
  steamvr_tracker.py
```

配置示例：

```yaml
teleop:
  type: steamvr_tracker
  id: wrist_tracker_tcp
  host: 0.0.0.0
  port: 8765
  expected_hz: 120
  timeout_s: 0.05
  wait_for_connection: true
  wait_for_first_sample: true
```

说明：Tracker TCP 接收频率为 120Hz；record loop 按数据集 FPS 采样时，每帧读取最近一条
tracker 样本。默认在 `teleop.connect()` 阶段硬阻塞，直到外部 PC 建立 TCP 连接并收到第一条可解析
tracker 样本后才进入正式采集流程。

### 4.4 Composite Teleoperator

LeRobot 的 `record_loop` 当前只接收一个 `teleop`，因此 P0 建议新增一个组合 teleoperator：

```text
src/lerobot/teleoperators/composite/
```

职责：

- 持有多个子 teleoperator。
- 统一 connect/disconnect。
- `get_action()` 合并手套和 tracker 输出。
- 为每个子设备保留 receive time / seq / valid。

配置示例：

```yaml
teleop:
  type: composite
  id: glove_tracker
  devices:
    glove:
      type: <glove_name>
      id: right_glove
      port: /dev/ttyUSB0
    tracker:
      type: steamvr_tracker
      id: wrist_tracker_tcp
      host: 0.0.0.0
      port: 8765
      expected_hz: 120
      timeout_s: 0.05
      wait_for_connection: true
      wait_for_first_sample: true
```

输出 action 合并后：

```text
action.hand_joints
action.left_wrist_tracker.x/y/z
action.left_wrist_tracker.rot_x/rot_y/rot_z
action.right_wrist_tracker.x/y/z
action.right_wrist_tracker.rot_x/rot_y/rot_z
action.glove_receive_time
action.tracker_receive_time
action.glove_seq
action.tracker_seq
action.glove_valid
action.tracker_valid
```

## 5. LeRobot Dataset Schema

P0 推荐最终 features：

```yaml
observation.images.wrist_fisheye:
  dtype: video
  shape: [1536, 1920, 3]

observation.state:
  dtype: float32
  names:
    - camera_receive_time
    - camera_seq

action:
  dtype: float32
  names:
    - hand_joint_00
    - hand_joint_01
    - ...
    - left_wrist_tracker.x
    - left_wrist_tracker.y
    - left_wrist_tracker.z
    - left_wrist_tracker.rot_x
    - left_wrist_tracker.rot_y
    - left_wrist_tracker.rot_z
    - right_wrist_tracker.x
    - right_wrist_tracker.y
    - right_wrist_tracker.z
    - right_wrist_tracker.rot_x
    - right_wrist_tracker.rot_y
    - right_wrist_tracker.rot_z
    - tracker_source_time
    - glove_receive_time
    - tracker_receive_time
    - glove_seq
    - tracker_seq
    - glove_valid
    - tracker_valid
```

说明：

- LeRobot 当前会将低维 action 聚合到一个 `action` 向量。
- 字段名称通过 `features["action"]["names"]` 保留。
- 时间戳字段 P0 可先用 float32，但长期建议评估 float64 或拆到平台 metadata/raw log。

## 6. YAML 配置需求

建议新增完整 demo 配置：

```text
configs/record/embodiment_free_p0.yaml
```

目标结构：

```yaml
robot:
  type: capture_rig
  id: wrist_fisheye_rig
  cameras:
    wrist_fisheye:
      type: opencv
      index_or_path: /dev/video0
      width: 1920
      height: 1536
      fps: 30
      fourcc: UYVY

teleop:
  type: composite
  id: glove_tracker
  devices:
    glove:
      type: <glove_name>
      id: right_glove
      port: /dev/ttyUSB0
    tracker:
      type: steamvr_tracker
      id: wrist_tracker_tcp
      host: 0.0.0.0
      port: 8765
      expected_hz: 120
      timeout_s: 0.05
      wait_for_connection: true
      wait_for_first_sample: true

dataset:
  repo_id: local/embodiment_free_p0
  root: data/embodiment_free_p0
  fps: 30
  num_episodes: 1
  episode_time_s: 30
  reset_time_s: 1
  single_task: wrist fisheye + glove + tracker capture
  push_to_hub: false
  video: true
  vcodec: h264

display_data: true
display_compressed_images: true
play_sounds: false
```

## 7. 同步策略

P0 没有设备时间戳，因此统一采用：

```text
timestamp_semantics: host_receive_time
clock: time.perf_counter
```

每个设备读线程维护：

```text
latest_value
receive_time
seq
valid
```

record loop 每帧记录：

```text
t_frame = time.perf_counter()
camera_receive_time
glove_receive_time
tracker_receive_time
glove_seq
tracker_seq
camera_seq
```

同步指标：

```text
camera_age_ms  = t_frame - camera_receive_time
glove_age_ms   = t_frame - glove_receive_time
tracker_age_ms = t_frame - tracker_receive_time

glove_tracker_skew_ms = glove_receive_time - tracker_receive_time
camera_tracker_skew_ms = camera_receive_time - tracker_receive_time
```

建议阈值：

| 等级 | 条件 |
|------|------|
| green | `< 33ms` |
| yellow | `33-66ms` |
| red | `> 66ms` |
| fatal | `> 150ms` 或连续 5 帧无更新 |

## 8. 平台 Metadata 需求

在 LeRobotDataset root 下新增：

```text
meta/platform/
  session_manifest.yaml
  device_manifest.yaml
  calibration_manifest.yaml
  quality_report.json
  sync_report.json
```

### 8.1 `session_manifest.yaml`

```yaml
session_id:
created_at:
operator_id:
task_id:
dataset_repo_id:
fps: 30
timestamp_semantics:
  camera: host_receive_time
  glove: host_receive_time
  tracker: host_receive_time
```

### 8.2 `device_manifest.yaml`

```yaml
devices:
  wrist_fisheye:
    type: opencv_camera
    path: /dev/video0
    width: 1920
    height: 1536
    fps: 30
    fourcc: UYVY
  glove:
    type:
    id:
    port:
  tracker:
    type: steamvr_tracker
    host: 0.0.0.0
    port: 8765
    expected_hz: 120
    timeout_s: 0.05
    wait_for_connection: true
    wait_for_first_sample: true
```

### 8.3 `calibration_manifest.yaml`

```yaml
camera:
  model: fisheye
  intrinsics_version:
  distortion_version:
tracker:
  frame_id: steamvr_standing
extrinsics:
  T_wrist_tracker_camera_version:
```

### 8.4 `sync_report.json`

```json
{
  "camera_age_ms": {"p50": 0, "p95": 0, "max": 0},
  "glove_age_ms": {"p50": 0, "p95": 0, "max": 0},
  "tracker_age_ms": {"p50": 0, "p95": 0, "max": 0},
  "glove_tracker_skew_ms": {"p50": 0, "p95": 0, "max": 0},
  "flags": []
}
```

## 9. Rerun 可视化需求

Rerun 中至少显示：

```text
observation.wrist_fisheye
action.hand_joints/*
action.left_wrist_tracker/*
action.right_wrist_tracker/*
sync.camera_age_ms
sync.glove_age_ms
sync.tracker_age_ms
valid.glove
valid.tracker
```

P0 可视化优先级：

1. 腕部相机图像。
2. 手套关节曲线。
3. Tracker 位置曲线。
4. valid/age/skew 同步指标。
5. 后续再做 3D 坐标系和轨迹。

正式采集建议允许关闭 Rerun：

```bash
uv run lerobot-record \
  --config_path=configs/record/embodiment_free_p0.yaml \
  --display_data=false
```

## 10. 质量检查需求

P0 质量 flags：

```text
camera_low_fps
camera_stale_frame
camera_black_frame
glove_timeout
glove_seq_stalled
glove_joint_jump
tracker_timeout
tracker_seq_stalled
tracker_tracking_lost
tracker_pose_jump
sync_skew_high
record_loop_slow
```

每个 episode 结束后生成：

```text
meta/platform/quality_report.json
```

最低报告内容：

```json
{
  "episode_index": 0,
  "num_frames": 900,
  "target_fps": 30,
  "actual_fps_mean": 29.2,
  "flags": [],
  "device_valid_ratio": {
    "camera": 1.0,
    "glove": 0.99,
    "tracker": 0.98
  }
}
```

## 11. 验收标准

### 11.1 单设备验收

腕部相机：

- 能录制 `1920x1536 UYVY @ 30fps`。
- 数据集中存在 `observation.images.wrist_fisheye`。
- 视频文件可被 `LeRobotDataset` 读取。

手套：

- 能连续输出 `hand_joints`。
- 有 `glove_receive_time`、`glove_seq`、`glove_valid`。
- 静止时曲线稳定，握拳/张开时曲线有合理变化。

Tracker：

- 能连续输出 `left_wrist_tracker.*` 和 `right_wrist_tracker.*`。
- 有 `tracker_receive_time`、`tracker_seq`、`tracker_valid`。
- 移动 tracker 时 position 和 rotation 三元组连续变化。

### 11.2 集成验收

- 三路设备同时 connect。
- `lerobot-record` 可完成 1 个 30 秒 episode。
- LeRobotDataset 包含图像和低维 action/state。
- Rerun 能显示图像和低维曲线。
- episode 结束后生成 platform metadata 和 quality/sync report。
- 无 fatal quality flag。

### 11.3 同步验收

在主机接收时间戳模式下：

- `camera_age_ms.p95 < 66ms`
- `glove_age_ms.p95 < 66ms`
- `tracker_age_ms.p95 < 50ms`
- `abs(glove_tracker_skew_ms).p95 < 66ms`
- 无连续 5 帧 seq 不更新。

## 12. 实施里程碑

### M1：当前状态

- `capture_rig` 已接入。
- `noop` 已接入。
- 腕部鱼眼相机 YAML 已可录制。

### M2：手套接入

- 实现或包装 glove teleoperator。
- 输出 hand joints + receive time + seq + valid。
- Rerun 显示手套曲线。

### M3：SteamVR Tracker 接入

- 实现 tracker teleoperator。
- 绑定 tracker serial。
- 输出 wrist pose + receive time + seq + valid。
- Rerun 显示 wrist pose 曲线。

### M4：Composite Teleoperator

- 组合 glove + tracker。
- 合并 action features。
- 支持 YAML 多设备配置。

### M5：同步与质量报告

- 增加 host receive timestamp。
- 增加 seq stale 检查。
- 输出 `sync_report.json` 和 `quality_report.json`。

### M6：平台 metadata

- 写入 session/device/calibration manifest。
- 明确 timestamp semantics。
- 为后续 retargeting 和训练追溯做准备。

## 13. 主要风险

| 风险 | 影响 | 缓解 |
|------|------|------|
| 高分辨率 + Rerun 导致掉帧 | 采集 FPS 不稳 | 正式采集关闭 Rerun，或低分辨率预览 |
| 手套没有设备时间戳 | 无法硬同步 | P0 用 host receive time + seq |
| SteamVR tracking lost | wrist pose 缺失/跳变 | 写 valid 字段和 quality flag |
| OpenCV 默认回退分辨率 | 数据分辨率不符合预期 | YAML 显式 `fourcc: UYVY` |
| action 向量字段过多 | 后续训练 mapping 混乱 | 严格维护 feature names 和 spec |
| float32 时间戳精度有限 | 长 session 精度风险 | P0 可用；长期考虑平台 raw log 用 float64 |

## 14. 后续训练对接预期

P0 采集数据暂不直接训练机器人策略，主要用于验证：

- 图像是否满足视觉模型输入要求。
- 手套关节是否稳定。
- tracker pose 是否可作为 wrist/hand pose 条件。
- 后处理是否能将 `hand_joints + left/right_wrist_tracker` 重定向为目标机器人 action。

后续训练格式可由 processor 转换：

```text
observation.images.wrist_fisheye
action.hand_joints + action.left_wrist_tracker + action.right_wrist_tracker
        ↓ retarget processor
action.robot_joint_targets / action.ee_delta
```
