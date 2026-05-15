# P0 腕部鱼眼相机最小验证 Demo Spec

> 面向“无本体数据采集平台”的第一阶段最小可运行闭环。  
> 状态：已在 `/dev/video0` 上完成 LeRobot 采集 smoke test。  
> 最后更新：2026-05-11。

## 1. 目标

本 demo 的目标是先打通单路腕部鱼眼摄像头的数据采集链路，验证以下能力：

- Linux 能稳定访问腕部相机设备。
- LeRobot 能以无本体方式接入相机。
- 采集参数可通过 YAML 配置。
- 数据可落为标准 `LeRobotDataset`。
- 录制时可用 Rerun 做实时画面调试。

本 demo 不包含手套、SteamVR tracker、跨设备同步、标定版本管理和质量报告。这些作为后续模块接入。

## 2. 当前硬件与采集参数

| 项目 | 当前值 |
|------|--------|
| 相机设备 | `/dev/video0` |
| V4L2/GStreamer caps | `UYVY, 1920x1536, 30fps` |
| OpenCV backend | `V4L2` |
| LeRobot camera key | `wrist_fisheye` |
| 输出特征 | `observation.images.wrist_fisheye` |
| 视频编码 | `h264` |
| 数据格式 | `LeRobotDataset v3` |

GStreamer 验证命令：

```bash
gst-launch-1.0 -v v4l2src device=/dev/video0 num-buffers=1 ! fakesink
```

预期 caps：

```text
video/x-raw, format=UYVY, width=1920, height=1536, framerate=30/1
```

OpenCV 必须显式设置 `fourcc: UYVY`，否则可能回退到 `1280x720`。

## 3. LeRobot 接入设计

### 3.1 `capture_rig` Robot

新增 `capture_rig` 作为无本体采集 rig：

```text
src/lerobot/robots/capture_rig/
  __init__.py
  config_capture_rig.py
  capture_rig.py
```

职责：

- 持有并连接相机。
- 在 `get_observation()` 中读取相机帧。
- 暴露 LeRobot 视觉 observation feature。
- `send_action()` 为 no-op，不控制任何实体。

当前 observation feature：

```text
observation.images.wrist_fisheye
shape: [1536, 1920, 3]
dtype: video
```

### 3.2 `noop` Teleoperator

新增 `noop` teleoperator：

```text
src/lerobot/teleoperators/noop/
  __init__.py
  config_noop.py
  noop.py
```

职责：

- 满足 `lerobot-record` 当前“录制必须有 teleop”的接口要求。
- `get_action()` 返回空字典。
- 不产生 action feature。

### 3.3 注册入口

以下文件已接入新类型：

```text
src/lerobot/robots/utils.py
src/lerobot/teleoperators/utils.py
src/lerobot/scripts/lerobot_record.py
```

## 4. YAML 配置

当前配置文件：

```text
configs/record/wrist_fisheye_capture.yaml
```

核心内容：

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
  type: noop
  id: noop

dataset:
  repo_id: local/wrist_fisheye_capture
  root: data/wrist_fisheye_capture
  fps: 30
  num_episodes: 1
  episode_time_s: 30
  reset_time_s: 1
  single_task: wrist fisheye capture test
  push_to_hub: false
  video: true
  vcodec: h264

display_data: true
display_compressed_images: true
play_sounds: false
```

## 5. 运行方式

### 5.1 环境初始化

```bash
source /home/udexreal/.local/bin/env
```

当前项目环境由 `uv` 管理，使用 Python 3.12：

```bash
uv run python --version
```

预期：

```text
Python 3.12.x
```

在受限 shell 中建议设置 cache 目录：

```bash
export UV_CACHE_DIR=/tmp/uv-cache
export HF_HOME=/tmp/hf-home
export HF_DATASETS_CACHE=/tmp/hf-datasets
```

### 5.2 正常录制

```bash
uv run lerobot-record --config_path=configs/record/wrist_fisheye_capture.yaml
```

### 5.3 临时覆盖参数

录制 1 个 5 秒 episode：

```bash
uv run lerobot-record \
  --config_path=configs/record/wrist_fisheye_capture.yaml \
  --dataset.num_episodes=1 \
  --dataset.episode_time_s=5
```

正式采集时如果不需要实时预览，建议关闭 Rerun，降低 CPU 压力：

```bash
uv run lerobot-record \
  --config_path=configs/record/wrist_fisheye_capture.yaml \
  --display_data=false
```

## 6. 数据产物

默认输出目录：

```text
data/wrist_fisheye_capture/
```

典型结构：

```text
data/
  chunk-000/file-000.parquet
meta/
  info.json
  stats.json
  tasks.parquet
  episodes/chunk-000/file-000.parquet
videos/
  observation.images.wrist_fisheye/chunk-000/file-000.mp4
```

数据集中应包含：

```text
observation.images.wrist_fisheye
timestamp
frame_index
episode_index
index
task_index
task
```

注意：当前 `timestamp` 仍为 LeRobot 默认的 `frame_index / fps`，不是相机硬件时间戳。

## 7. Rerun 可视化

配置项：

```yaml
display_data: true
display_compressed_images: true
```

本机有图形界面时，LeRobot 会自动 spawn Rerun viewer。

远程 viewer 模式可使用：

```bash
uv run lerobot-record \
  --config_path=configs/record/wrist_fisheye_capture.yaml \
  --display_ip=<rerun_server_ip> \
  --display_port=<rerun_server_port>
```

高分辨率下 Rerun 可能导致 loop 低于 30Hz。正式采集建议：

- 调试时开启 Rerun。
- 正式录制时关闭 Rerun。
- 或单独使用低分辨率预览配置。

## 8. 验收标准

本 demo 的最小验收标准：

- `/dev/video0` 可被 V4L2/GStreamer 枚举。
- OpenCV 可读取 `1920x1536 UYVY @ 30fps`。
- `lerobot-record --config_path=...` 能完成至少 1 个 episode。
- 输出目录中存在 `videos/observation.images.wrist_fisheye/...mp4`。
- `LeRobotDataset` 可加载该 dataset。
- Rerun 可显示实时 `observation.wrist_fisheye` 图像。

## 9. 已知限制

1. **无设备级时间戳**
   - 当前仅有 LeRobot 统一帧时间戳。
   - 后续需要增加 `camera_receive_time` 或 `camera_capture_time`。

2. **Rerun 高分辨率压力较大**
   - `1920x1536` + Rerun 可能低于 30Hz。
   - 可通过关闭 `display_data` 或降低预览分辨率缓解。

3. **仅验证单路视觉**
   - 尚未接入手套和 SteamVR tracker。
   - 尚未做跨设备同步检测。

4. **未接入标定系统**
   - 当前没有相机内参、鱼眼畸变参数和相机到腕部坐标系外参。

5. **未生成平台级 metadata**
   - 当前只有 LeRobot 原生 metadata。
   - 后续需要补 `session_manifest.yaml`、`device_manifest.yaml`、`calibration_manifest.yaml`、`quality_report.json`。

## 10. 后续模块对接预留

### 10.1 手套接入

建议新增：

```text
src/lerobot/teleoperators/<glove_name>/
```

输出 action：

```text
action.hand_joints
action.glove_receive_time
action.glove_seq
```

### 10.2 SteamVR Tracker 接入

建议新增：

```text
src/lerobot/teleoperators/steamvr_tracker/
```

输出 action：

```text
action.wrist_pose     # [x, y, z, qx, qy, qz, qw]
action.tracker_valid
action.tracker_receive_time
action.tracker_seq
```

### 10.3 同步监控

建议新增采集时质量指标：

```text
camera_age_ms
glove_age_ms
tracker_age_ms
glove_tracker_skew_ms
camera_tracker_skew_ms
```

输出：

```text
meta/platform/quality_report.json
meta/platform/sync_plot.png
```

### 10.4 标定接入

建议补充：

```text
meta/platform/calibration_manifest.yaml
```

至少记录：

```yaml
camera_model: fisheye
intrinsics_version:
distortion_version:
T_wrist_tracker_camera_version:
calibration_raw_uri:
```

## 11. 常见问题

### 11.1 OpenCV 报实际宽度是 1280

错误示例：

```text
RuntimeError: OpenCVCamera(/dev/video0) failed to set capture_width=1920 (actual_width=1280)
```

解决：

```yaml
fourcc: UYVY
```

原因：GStreamer 会自动协商到 `UYVY 1920x1536`，但 OpenCV 默认可能协商到 `1280x720`。显式设置 FOURCC 后可使用高分辨率。

### 11.2 `draccus` CLI 报错

原因可能是环境使用了 Python 3.14。当前项目环境应固定 Python 3.12：

```bash
uv sync --locked --extra core_scripts --python 3.12
```

### 11.3 录制低于 30Hz

常见原因：

- Rerun 实时显示高分辨率图像。
- 图像压缩/编码占用 CPU。
- 写盘速度不足。

处理顺序：

1. 关闭 `display_data`。
2. 降低分辨率到 `1280x720` 做预览。
3. 尝试 `streaming_encoding=true`。
4. 增加 image writer threads。
