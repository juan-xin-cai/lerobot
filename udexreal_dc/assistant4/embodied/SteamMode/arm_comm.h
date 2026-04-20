#ifndef ARM_COMM_H
#define ARM_COMM_H

#include <cstddef>
#include <cstdint>

struct ArmPoseState {
    float pos[3];
    float quat_wxyz[4];
};

struct ArmAnalogState {
    float trigger;
    float grip;
    float joystick_x;
    float joystick_y;
};

struct ArmButtonState {
    bool trigger;
    bool grip;
    bool a;
    bool b;
    bool menu;
    bool joystick_press;
};

struct ArmEventState {
    bool reset_robot;
    bool recenter_vr;
};

struct ArmControlFrame {
    std::int64_t timestamp_ms;
    const char* frame;
    bool tracking_ok;
    bool following;
    ArmPoseState pose;
    ArmAnalogState analog;
    ArmButtonState buttons;
    ArmEventState events;
};

// 初始化TCP连接（在程序启动时调用一次）
void init_arm_comm();

// 发送完整 VR 控制帧到机械臂服务端，返回是否成功
bool send_pose_to_arm(const ArmControlFrame& control_frame);

// 关闭连接（程序退出时调用）
void close_arm_comm();

#endif
