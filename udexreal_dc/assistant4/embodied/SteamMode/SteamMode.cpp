      
// SteamMode.cpp
//

#include "stdafx.h"
#include "arm_comm.h"
#include <chrono>
#include <cmath>
#include <string.h>
#include <stdlib.h>

#include "deepoon_sdk_native.h"
#include "dpn_peripheral_interface_common.h"
#include "dpn_peripheral_interface_sdk.h"

#define DEVICE_HMD_INDEX 0
#define DEVICE_CONTROLLER_LEFT_INDEX 1
#define DEVICE_CONTROLLER_RIGHT_INDEX 2
#define DEVICE_CONTROLLER_THIRD_INDEX 3
#define DEVICE_STATION_MASTER_INDEX 4
#define DEVICE_STATION_SLAVE_INDEX 5
#define MAX_DEVICE_COUNT 6
#define HAND_LEFT 0
#define HAND_RIGHT 1

typedef struct _DpnnHmdRecord {
    DpnnDeviceTransform hmdTransform;
} DpnnHmdRecord;

#ifdef _WIN32
#define strncat strncat_s
#endif

static DPNP_DEVICE g_devices[MAX_DEVICE_COUNT] = {0};
static int g_states[MAX_DEVICE_COUNT] = {0};
static DpnnControllerRecordOriginal g_controller_raw[MAX_CONTROLLER_COUNT];
static DpnnHmdRecord g_hmd_record;

void ClearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

#ifndef _WIN32
#include <unistd.h>
void Sleep(int ms) {
    usleep(ms * 1000);
}
#endif

bool recenter()
{
    const int bufLen = 128;
    char buf[bufLen] = {};
    int id = 27;
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &id, sizeof(id));
    return DpnpSetDeviceAttribute(g_devices[0], DPNP_VALUE_TYPE_COMMON_EVENT - DPNP_VALUE_TYPE_ATTRIBUTE, buf, bufLen);
}


dpnnInstance g_instance;
static void DpnpHandleControllerEventSteamCB(DPNP_DEVICE device,
                                             int event_mask,
                                             void* user_data) {
    if (!g_instance)
        return;

    int hand = -1;
    int device_index = -1;
    if (device == g_devices[DEVICE_CONTROLLER_LEFT_INDEX]) {
        hand = HAND_LEFT;
        device_index = DEVICE_CONTROLLER_LEFT_INDEX;
    } else if (device == g_devices[DEVICE_CONTROLLER_RIGHT_INDEX]) {
        hand = HAND_RIGHT;
        device_index = DEVICE_CONTROLLER_RIGHT_INDEX;
    } else if (device == g_devices[DEVICE_CONTROLLER_THIRD_INDEX]) {
        hand = 2;
        device_index = DEVICE_CONTROLLER_THIRD_INDEX;
    } else {
        return;
    }

    // set device connection state
    //
    if (event_mask &
        (DPNP_EVENT_TYPE_BASE_A_TRACK | DPNP_EVENT_TYPE_BASE_A_UNTRACK |
         DPNP_EVENT_TYPE_BASE_B_TRACK | DPNP_EVENT_TYPE_BASE_B_UNTRACK)) {
        if (event_mask & DPNP_EVENT_TYPE_BASE_A_TRACK) {
            g_states[device_index] |= DPNP_DEVICE_STATUS_BASE_A_TRACK;
            g_states[device_index] &= ~DPNP_DEVICE_STATUS_BASE_A_UNTRACK;
            g_controller_raw[hand].connect_state |= DPNP_DEVICE_STATUS_BASE_A_TRACK;
            g_controller_raw[hand].connect_state &=
                ~DPNP_DEVICE_STATUS_BASE_A_UNTRACK;
        }

        if (event_mask & DPNP_EVENT_TYPE_BASE_A_UNTRACK) {
            g_states[device_index] |= DPNP_DEVICE_STATUS_BASE_A_UNTRACK;
            g_states[device_index] &= ~DPNP_DEVICE_STATUS_BASE_A_TRACK;
            g_controller_raw[hand].connect_state |= DPNP_DEVICE_STATUS_BASE_A_UNTRACK;
            g_controller_raw[hand].connect_state &= ~DPNP_DEVICE_STATUS_BASE_A_TRACK;
        }

        if (event_mask & DPNP_EVENT_TYPE_BASE_B_TRACK) {
            g_states[device_index] |= DPNP_DEVICE_STATUS_BASE_B_TRACK;
            g_states[device_index] &= ~DPNP_DEVICE_STATUS_BASE_B_UNTRACK;
            g_controller_raw[hand].connect_state |= DPNP_DEVICE_STATUS_BASE_B_TRACK;
            g_controller_raw[hand].connect_state &=
                ~DPNP_DEVICE_STATUS_BASE_B_UNTRACK;
        }

        if (event_mask & DPNP_EVENT_TYPE_BASE_B_UNTRACK) {
            g_states[device_index] |= DPNP_DEVICE_STATUS_BASE_B_UNTRACK;
            g_states[device_index] &= ~DPNP_DEVICE_STATUS_BASE_B_TRACK;
            g_controller_raw[hand].connect_state |= DPNP_DEVICE_STATUS_BASE_B_UNTRACK;
            g_controller_raw[hand].connect_state &= ~DPNP_DEVICE_STATUS_BASE_B_TRACK;
        }
    }

    if (event_mask &
        (DPNP_EVENT_TYPE_POSE_UPDATE | DPNP_EVENT_TYPE_POSITION_UPDATE |
         DPNP_EVENT_TYPE_TIME_UPDATE)) {
        bool bRet =
            DpnpReadDeviceTransform(device, 0, &g_controller_raw[hand].transform);
        if (!bRet) {
            if (event_mask & DPNP_EVENT_TYPE_POSE_UPDATE) {
                bRet = DpnpReadDevicePose(device, 0,
                                          g_controller_raw[hand].transform.pose);
                if (!bRet) {
                    printf("DpnpReadDevicePose Failed\n");
                    return;
                }
            }
            if (event_mask & DPNP_EVENT_TYPE_POSITION_UPDATE) {
                bRet = DpnpReadDevicePosition(
                    device, 0, g_controller_raw[hand].transform.position);
                if (!bRet) {
                    printf("DpnpReadDevicePosition Failed\n");
                    return;
                }
            }
            if (event_mask & DPNP_EVENT_TYPE_TIME_UPDATE) {
                g_controller_raw[hand].transform.time[0] =
                    DpnpReadDeviceTime(device, 1);
            }
        }
    }

    if (event_mask &
        (DPNP_EVENT_TYPE_BUTTON_UPDATE | DPNP_EVENT_TYPE_AXIS_UPDATE)) {
        g_controller_raw[hand].button_mask =
            DpnpReadDeviceButton(device, DPNP_ELITE_BUTTON_PRESS);
        bool bRet = DpnpReadDeviceAnalog(device, 0, g_controller_raw[hand].analog);
        if (!bRet) {
            g_controller_raw[hand].analog[DPNP_ELITE_CONTROLLER_ANALOG_TRIGGER] =
                DpnpReadDeviceAxis(device, DPNP_ELITE_CONTROLLER_ANALOG_TRIGGER);
            g_controller_raw[hand].analog[DPNP_ELITE_CONTROLLER_ANALOG_GRIP] =
                DpnpReadDeviceAxis(device, DPNP_ELITE_CONTROLLER_ANALOG_GRIP);
            g_controller_raw[hand].analog[DPNP_ELITE_CONTROLLER_ANALOG_JOYSTICK_X] =
                DpnpReadDeviceAxis(device, DPNP_ELITE_CONTROLLER_ANALOG_JOYSTICK_X);
            g_controller_raw[hand].analog[DPNP_ELITE_CONTROLLER_ANALOG_JOYSTICK_Y] =
                DpnpReadDeviceAxis(device, DPNP_ELITE_CONTROLLER_ANALOG_JOYSTICK_Y);
        }
    }
}

static void DpnpHandleHmdEventSteamCB(DPNP_DEVICE device,
                                      int event_mask,
                                      void* user_data) {
    static DpnnDevicePose s_pose = {0};
    static DpnnDevicePosition s_position = {0};
    DpnnDeviceTransform* p_transform = NULL;

    if (!g_instance || device != g_devices[DEVICE_HMD_INDEX])
        return;
    p_transform = &g_hmd_record.hmdTransform;

    if (event_mask &
        (DPNP_EVENT_TYPE_USB_PLUGIN | DPNP_EVENT_TYPE_USB_UNPLUG |
         DPNP_EVENT_TYPE_BASE_A_TRACK | DPNP_EVENT_TYPE_BASE_A_UNTRACK |
         DPNP_EVENT_TYPE_BASE_B_TRACK | DPNP_EVENT_TYPE_BASE_B_UNTRACK)) {
        // set device connection state
        //
        if (event_mask & DPNP_EVENT_TYPE_USB_PLUGIN) {
            g_states[DEVICE_HMD_INDEX] |= DPNP_DEVICE_STATUS_USB_PLUGIN;
            g_states[DEVICE_HMD_INDEX] &= ~DPNP_DEVICE_STATUS_USB_UNPLUG;
            printf("HMD USB plugin\n");
        }

        if (event_mask & DPNP_EVENT_TYPE_USB_UNPLUG) {
            g_states[DEVICE_HMD_INDEX] |= DPNP_DEVICE_STATUS_USB_UNPLUG;
            g_states[DEVICE_HMD_INDEX] &= ~DPNP_DEVICE_STATUS_USB_PLUGIN;
            printf("HMD USB unplug\n");
        }

        if (event_mask & DPNP_EVENT_TYPE_BASE_A_TRACK) {
            g_states[DEVICE_HMD_INDEX] |= DPNP_DEVICE_STATUS_BASE_A_TRACK;
            g_states[DEVICE_HMD_INDEX] &= ~DPNP_DEVICE_STATUS_BASE_A_UNTRACK;
        }

        if (event_mask & DPNP_EVENT_TYPE_BASE_A_UNTRACK) {
            g_states[DEVICE_HMD_INDEX] |= DPNP_DEVICE_STATUS_BASE_A_UNTRACK;
            g_states[DEVICE_HMD_INDEX] &= ~DPNP_DEVICE_STATUS_BASE_A_TRACK;
        }

        if (event_mask & DPNP_EVENT_TYPE_BASE_B_TRACK) {
            g_states[DEVICE_HMD_INDEX] |= DPNP_DEVICE_STATUS_BASE_B_TRACK;
            g_states[DEVICE_HMD_INDEX] &= ~DPNP_DEVICE_STATUS_BASE_B_UNTRACK;
        }

        if (event_mask & DPNP_EVENT_TYPE_BASE_B_UNTRACK) {
            g_states[DEVICE_HMD_INDEX] |= DPNP_DEVICE_STATUS_BASE_B_UNTRACK;
            g_states[DEVICE_HMD_INDEX] &= ~DPNP_DEVICE_STATUS_BASE_B_TRACK;
        }

        if (g_states[DEVICE_HMD_INDEX] & DPNP_DEVICE_STATUS_USB_PLUGIN) {
            p_transform->is_connected = 1;
            p_transform->is_valid = 1;
        } else {
            p_transform->is_connected = 0;
            p_transform->is_valid = 0;
        }
    }

    if (event_mask &
        (DPNP_EVENT_TYPE_POSE_UPDATE | DPNP_EVENT_TYPE_POSITION_UPDATE |
         DPNP_EVENT_TYPE_TIME_UPDATE)) {
        bool bRet = DpnpReadDeviceTransform(device, 0, &p_transform->transform);
        if (!bRet) {
            if (event_mask & DPNP_EVENT_TYPE_POSE_UPDATE) {
                bRet = DpnpReadDevicePose(device, 0, p_transform->transform.pose);
                if (!bRet) {
                    printf("DpnpReadDevicePose Failed\n");
                    return;
                }
            }
            if (event_mask & DPNP_EVENT_TYPE_POSITION_UPDATE) {
                bRet =
                    DpnpReadDevicePosition(device, 0, p_transform->transform.position);
                if (!bRet) {
                    printf("DpnpReadDevicePosition Failed\n");
                    return;
                }
            }
            if (event_mask & DPNP_EVENT_TYPE_TIME_UPDATE) {
                p_transform->transform.time[0] = DpnpReadDeviceTime(device, 1);
            }
        }
    }
}

static char* getButtonPressedStr(int button_mask, int hand) {
    static char buttons[256];
    int keys = 0;
    buttons[0] = '\0';
    if (button_mask & DPNP_ELITE_BUTTONMASK_HOME_MENU) {
        if (hand == HAND_LEFT)
            strncat(buttons, "Menu ", 256);
        else if (hand == HAND_RIGHT)
            strncat(buttons, "DPVR(Home) ", 256);
        else
            strncat(buttons, "HOME_MENU ", 256);
        ++keys;
    }
    if (button_mask & DPNP_ELITE_BUTTONMASK_XA) {
        if (hand == HAND_LEFT)
            strncat(buttons, "X ", 256);
        else if (hand == HAND_RIGHT)
            strncat(buttons, "A ", 256);
        else
            strncat(buttons, "XA ", 256);
        ++keys;
    }
    if (button_mask & DPNP_ELITE_BUTTONMASK_YB) {
        if (hand == HAND_LEFT)
            strncat(buttons, "Y ", 256);
        else if (hand == HAND_RIGHT)
            strncat(buttons, "B ", 256);
        else
            strncat(buttons, "YB ", 256);
        ++keys;
    }
    if (button_mask & DPNP_ELITE_BUTTONMASK_GRIP) {
        strncat(buttons, "GRIP ", 256);
        ++keys;
    }
    if (button_mask & DPNP_ELITE_BUTTONMASK_TRIGER) {
        strncat(buttons, "TRIGGER ", 256);
        ++keys;
    }
    if (button_mask & DPNP_ELITE_BUTTONMASK_JOYSTICK) {
        strncat(buttons, "JOYSTICK ", 256);
        ++keys;
    }
    if (0 == keys) {
        strncat(buttons, "NO KEY is pressed", 256);
    } else if (1 == keys) {
        strncat(buttons, "is pressed", 256);
    } else {
        strncat(buttons, "are pressed", 256);
    }
    return buttons;
}


// True trans
void transformVRToModel(float vr[3], float model[3]) {
    model[0] = -vr[2];
    model[1] = -vr[0];
    model[2] = vr[1];
}



// 将 VR 手柄的四元数转换到模型坐标系（与位置映射保持一致）
static void transformQuatVRToModel(const float q_vr[4], float q_model[4]) {
    q_model[0] = q_vr[0];          
    q_model[1] = -q_vr[3];          
    q_model[2] = -q_vr[1];          
    q_model[3] = q_vr[2];          
} 

// Sim trans 

/*void transformVRToModel(float vr[3], float model[3]) {
    model[0] = vr[2];
    model[1] = vr[0];
    model[2] = vr[1];
}

// 将 VR 手柄的四元数转换到模型坐标系（与位置映射保持一致）
static void transformQuatVRToModel(const float q_vr[4], float q_model[4]) {
    q_model[0] = q_vr[0];          
    q_model[1] = q_vr[3];          
    q_model[2] = q_vr[1];          
    q_model[3] = q_vr[2];          
}

*/

// 四元数乘法: q = q1 * q2
static void quatMultiply(const float q1[4], const float q2[4], float q[4]) {
    q[0] = q1[0]*q2[0] - q1[1]*q2[1] - q1[2]*q2[2] - q1[3]*q2[3];
    q[1] = q1[0]*q2[1] + q1[1]*q2[0] + q1[2]*q2[3] - q1[3]*q2[2];
    q[2] = q1[0]*q2[2] - q1[1]*q2[3] + q1[2]*q2[0] + q1[3]*q2[1];
    q[3] = q1[0]*q2[3] + q1[1]*q2[2] - q1[2]*q2[1] + q1[3]*q2[0];
}

// 四元数共轭
static void quatConjugate(const float q[4], float q_conj[4]) {
    q_conj[0] = q[0];
    q_conj[1] = -q[1];
    q_conj[2] = -q[2];
    q_conj[3] = -q[3];
}

// 四元数归一化
static void quatNormalize(float q[4]) {
    float norm = sqrt(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
    if (norm > 1e-6) {
        q[0] /= norm;
        q[1] /= norm;
        q[2] /= norm;
        q[3] /= norm;
    }
}


static bool is_position_safe(const float pos[3], const float center[3], float radius) {
    float dx = pos[0] - center[0];
    float dy = pos[1] - center[1];
    float dz = pos[2] - center[2];
    float dist = sqrt(dx*dx + dy*dy + dz*dz);
    return dist <= radius;
}

static std::int64_t current_timestamp_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

static bool controller_tracking_ok(int device_state) {
    return (device_state & (DPNP_DEVICE_STATUS_BASE_A_TRACK |
                            DPNP_DEVICE_STATUS_BASE_B_TRACK)) != 0;
}

static ArmControlFrame build_arm_control_frame(const float model_pos[3],
                                               const float model_quat[4],
                                               bool tracking_ok,
                                               bool following,
                                               const DpnnControllerRecordOriginal* controller,
                                               bool reset_robot_event,
                                               bool recenter_vr_event) {
    ArmControlFrame control_frame = {};
    control_frame.timestamp_ms = current_timestamp_ms();
    control_frame.frame = "airbot_base";
    control_frame.tracking_ok = tracking_ok;
    control_frame.following = following;
    memcpy(control_frame.pose.pos, model_pos, sizeof(control_frame.pose.pos));
    memcpy(control_frame.pose.quat_wxyz, model_quat, sizeof(control_frame.pose.quat_wxyz));

    if (controller != NULL) {
        control_frame.analog.trigger =
            controller->analog[DPNP_ELITE_CONTROLLER_ANALOG_TRIGGER];
        control_frame.analog.grip =
            controller->analog[DPNP_ELITE_CONTROLLER_ANALOG_GRIP];
        control_frame.analog.joystick_x =
            controller->analog[DPNP_ELITE_CONTROLLER_ANALOG_JOYSTICK_X];
        control_frame.analog.joystick_y =
            controller->analog[DPNP_ELITE_CONTROLLER_ANALOG_JOYSTICK_Y];

        const int button_mask = controller->button_mask;
        control_frame.buttons.trigger =
            (button_mask & DPNP_ELITE_BUTTONMASK_TRIGER) != 0;
        control_frame.buttons.grip =
            (button_mask & DPNP_ELITE_BUTTONMASK_GRIP) != 0;
        control_frame.buttons.a =
            (button_mask & DPNP_ELITE_BUTTONMASK_XA) != 0;
        control_frame.buttons.b =
            (button_mask & DPNP_ELITE_BUTTONMASK_YB) != 0;
        control_frame.buttons.menu =
            (button_mask & DPNP_ELITE_BUTTONMASK_HOME_MENU) != 0;
        control_frame.buttons.joystick_press =
            (button_mask & DPNP_ELITE_BUTTONMASK_JOYSTICK) != 0;
    }

    control_frame.events.reset_robot = reset_robot_event;
    control_frame.events.recenter_vr = recenter_vr_event;
    return control_frame;
}

int main(int argc, char* argv[]) {
    init_arm_comm();

    // ========== 解析外部定制参数 ==========
//true pose
   float init_model_pos[3]   = {0.0401f, 0.0000f, 0.2133f};
   float init_model_quat[4]  = {1.0000f, -0.0002f, 0.0002f, 0.0000f};

//sim zero pose
//    float init_model_pos[3]   = {0.291453f, -1.88862e-07f, 0.999061f};
//    float init_model_quat[4]  = {1.0f, 7.47988e-07f, -7.31796e-06f, 8.44359e-07f};



// piper pose right
//    float init_model_pos[3]   = {-0.307297f, 0.168898f, 0.781412f};
//    float init_model_quat[4]  = {-0.402973f, -0.402979f, 0.581046f, 0.581039f};

  
    //airbot pose right 
/*     float init_model_pos[3]   = {-0.370783f, 0.228783f, 0.779998f};
    float init_model_quat[4]  = {-0.338998f, -0.339004f, 0.620551f, 0.620543f}; */
 

    float change_rate = 0.8f;   

    float safe_radius = 1.0f;

    if (argc >= 8) {
        init_model_pos[0] = atof(argv[1]);
        init_model_pos[1] = atof(argv[2]);
        init_model_pos[2] = atof(argv[3]);
        init_model_quat[0] = atof(argv[4]);
        init_model_quat[1] = atof(argv[5]);
        init_model_quat[2] = atof(argv[6]);
        init_model_quat[3] = atof(argv[7]);
        if (argc >= 9) safe_radius = atof(argv[8]);
        printf("Using custom init: pos=(%.4f,%.4f,%.4f) quat=(%.4f,%.4f,%.4f,%.4f) radius=%.4f\n",
               init_model_pos[0], init_model_pos[1], init_model_pos[2],
               init_model_quat[0], init_model_quat[1], init_model_quat[2], init_model_quat[3],
               safe_radius);
    } else {
        printf("Using default init: pos=(%.4f,%.4f,%.4f) quat=(%.4f,%.4f,%.4f,%.4f) radius=%.4f\n",
               init_model_pos[0], init_model_pos[1], init_model_pos[2],
               init_model_quat[0], init_model_quat[1], init_model_quat[2], init_model_quat[3],
               safe_radius);
        printf("Usage: %s pos_x pos_y pos_z quat_w quat_x quat_y quat_z [radius]\n", argv[0]);
    }

    // 当前机械臂位姿
    float model_pos[3];
    float model_quat[4];
    memcpy(model_pos, init_model_pos, sizeof(model_pos));
    memcpy(model_quat, init_model_quat, sizeof(model_quat));
    ArmControlFrame initial_frame =
        build_arm_control_frame(model_pos, model_quat, false, false, NULL, false, false);
    send_pose_to_arm(initial_frame);

    // ========== DPNN 初始化 ==========
    g_instance = DpnnInit(1, DPNN_UM_SENSOR_ONLY, NULL, DPNN_DEVICE_DX11, NULL);
    if (g_instance == NULL) {
        printf("Error: DpnnInit return NULL");
        return 0;
    }

    int event_mask_hmd =
        DPNP_EVENT_TYPE_POSE_UPDATE | DPNP_EVENT_TYPE_POSITION_UPDATE |
        DPNP_EVENT_TYPE_TIME_UPDATE | DPNP_EVENT_TYPE_AXIS_UPDATE |
        DPNP_EVENT_TYPE_USB_PLUGIN | DPNP_EVENT_TYPE_USB_UNPLUG |
        DPNP_EVENT_TYPE_BASE_A_TRACK | DPNP_EVENT_TYPE_BASE_A_UNTRACK |
        DPNP_EVENT_TYPE_BASE_B_TRACK | DPNP_EVENT_TYPE_BASE_B_UNTRACK;

    do {
        int hmd_device_count = DpnpQueryDeviceCount(DPNP_DEVICE_TYPE_HEAD_TRACKER);
        if (hmd_device_count <= 0) {
            printf("DpnpQueryDeviceCount DPNP_DEVICE_TYPE_HEAD_TRACKER Failed\n");
            break;
        }
        const char* device_id = DpnpGetDeviceId(DPNP_DEVICE_TYPE_HEAD_TRACKER, 0);
        if (device_id == NULL) {
            printf("DpnpGetDeviceId DPNP_DEVICE_TYPE_HEAD_TRACKER Failed\n");
            break;
        }
        g_devices[DEVICE_HMD_INDEX] = DpnpOpenDevice(device_id);
        if (g_devices[DEVICE_HMD_INDEX] == NULL) {
            printf("DpnpOpenDevice DPNP_DEVICE_TYPE_HEAD_TRACKER Failed\n");
            break;
        }
        int state;
        DpnpReadDeviceAttribute(
            g_devices[0],
            DPNP_VALUE_TYPE_ATTRIBUTE_DEVICE_STATUS - DPNP_VALUE_TYPE_ATTRIBUTE,
            &state, sizeof(int));
        if (!(state & DPNP_DEVICE_STATUS_USB_PLUGIN)) {
            printf("DPVR HMD is connected, exit\n, %d", state);
            break;
        }
        g_states[DEVICE_HMD_INDEX] = state;
        DpnpRegisterEventNotificationFunction(g_devices[DEVICE_HMD_INDEX],
                                              DpnpHandleHmdEventSteamCB,
                                              event_mask_hmd, NULL);

        int event_mask_other =
            DPNP_EVENT_TYPE_POSE_UPDATE | DPNP_EVENT_TYPE_POSITION_UPDATE |
            DPNP_EVENT_TYPE_TIME_UPDATE | DPNP_EVENT_TYPE_AXIS_UPDATE |
            DPNP_EVENT_TYPE_BUTTON_UPDATE | DPNP_EVENT_TYPE_CONNECT |
            DPNP_EVENT_TYPE_DISCONNECT | DPNP_EVENT_TYPE_USB_PLUGIN |
            DPNP_EVENT_TYPE_USB_UNPLUG | DPNP_EVENT_TYPE_BASE_A_TRACK |
            DPNP_EVENT_TYPE_BASE_A_UNTRACK | DPNP_EVENT_TYPE_BASE_B_TRACK |
            DPNP_EVENT_TYPE_BASE_B_UNTRACK;

        int lhc_device_count = DpnpQueryDeviceCount(DPNP_DEVICE_TYPE_LEFT_CONTROLLER);
        if (lhc_device_count <= 0) {
            printf("DpnpQueryDeviceCount DPNP_DEVICE_TYPE_LEFT_CONTROLLER return 0\n");
        } else {
            device_id = DpnpGetDeviceId(DPNP_DEVICE_TYPE_LEFT_CONTROLLER, 0);
            if (device_id == NULL) {
                printf("DpnpGetDeviceId DPNP_DEVICE_TYPE_LEFT_CONTROLLER Failed\n");
                break;
            }
            g_devices[DEVICE_CONTROLLER_LEFT_INDEX] = DpnpOpenDevice(device_id);
            if (g_devices[DEVICE_CONTROLLER_LEFT_INDEX] == NULL) {
                printf("DpnpOpenDevice DPNP_DEVICE_TYPE_LEFT_CONTROLLER Failed\n");
                break;
            }
            DpnpRegisterEventNotificationFunction(
                g_devices[DEVICE_CONTROLLER_LEFT_INDEX],
                DpnpHandleControllerEventSteamCB, event_mask_other, NULL);
        }

        int rhc_device_count = DpnpQueryDeviceCount(DPNP_DEVICE_TYPE_RIGHT_CONTROLLER);
        if (rhc_device_count <= 0) {
            printf("DpnpQueryDeviceCount DPNP_DEVICE_TYPE_RIGHT_CONTROLLER return 0\n");
        } else {
            device_id = DpnpGetDeviceId(DPNP_DEVICE_TYPE_RIGHT_CONTROLLER, 0);
            if (device_id == NULL) {
                printf("DpnpGetDeviceId DPNP_DEVICE_TYPE_RIGHT_CONTROLLER Failed\n");
                break;
            }
            g_devices[DEVICE_CONTROLLER_RIGHT_INDEX] = DpnpOpenDevice(device_id);
            if (g_devices[DEVICE_CONTROLLER_RIGHT_INDEX] == NULL) {
                printf("DpnpOpenDevice DPNP_DEVICE_TYPE_RIGHT_CONTROLLER Failed\n");
                break;
            }
            DpnpRegisterEventNotificationFunction(
                g_devices[DEVICE_CONTROLLER_RIGHT_INDEX],
                DpnpHandleControllerEventSteamCB, event_mask_other, NULL);
        }
        printf("DpnnInit Success\n");

        // 跟随状态变量
        static bool following = false;
        static float start_hand_pos[3];
        static float start_model_pos[3];
        static float start_hand_quat_model[4];
        static float start_model_quat[4];
        static bool previous_b_pressed = false;
        static bool wait_trigger_release_after_reset = false;

        while (true) {
            ClearScreen();
            bool tracking_ok = false;
            bool reset_robot_event = false;
            bool recenter_vr_event = false;
            bool current_b_pressed = false;
            const DpnnControllerRecordOriginal* right_controller = NULL;

            if (g_states[DEVICE_HMD_INDEX] & DPNP_DEVICE_STATUS_USB_PLUGIN) {
                printf(
                    "headset pose = (%f, %f, %f, "
                    "%f)\nheadset position = (%f, %f, %f)\n",
                    g_hmd_record.hmdTransform.transform.pose[1],
                    g_hmd_record.hmdTransform.transform.pose[2],
                    g_hmd_record.hmdTransform.transform.pose[3],
                    g_hmd_record.hmdTransform.transform.pose[0],
                    g_hmd_record.hmdTransform.transform.position[0],
                    g_hmd_record.hmdTransform.transform.position[1],
                    g_hmd_record.hmdTransform.transform.position[2]);

                printf("\n\n\n");
                // left controller display
                if (g_states[DEVICE_CONTROLLER_LEFT_INDEX] &
                    (DPNP_DEVICE_STATUS_BASE_A_TRACK |
                     DPNP_DEVICE_STATUS_BASE_B_TRACK)) {
                    printf(
                        "left controller is connected\nleft pose = (%f, %f, %f, "
                        "%f)\nleft position = (%f, %f, %f)\nleft %s\n",
                        g_controller_raw[HAND_LEFT].transform.pose[1],
                        g_controller_raw[HAND_LEFT].transform.pose[2],
                        g_controller_raw[HAND_LEFT].transform.pose[3],
                        g_controller_raw[HAND_LEFT].transform.pose[0],
                        g_controller_raw[HAND_LEFT].transform.position[0],
                        g_controller_raw[HAND_LEFT].transform.position[1],
                        g_controller_raw[HAND_LEFT].transform.position[2],
                        getButtonPressedStr(g_controller_raw[HAND_LEFT].button_mask,
                                            HAND_LEFT));
                } else {
                    printf("left controller is disconnected\n\n\n\n");
                }
                printf("\n\n\n");

                // right controller handling
                if (controller_tracking_ok(g_states[DEVICE_CONTROLLER_RIGHT_INDEX])) {
                    tracking_ok = true;
                    right_controller = &g_controller_raw[HAND_RIGHT];

                    printf(
                        "right controller is connected\nright pose = (%f, %f, %f, "
                        "%f)\nright position = (%f, %f, %f)\nright %s\n",
                        g_controller_raw[HAND_RIGHT].transform.pose[1],
                        g_controller_raw[HAND_RIGHT].transform.pose[2],
                        g_controller_raw[HAND_RIGHT].transform.pose[3],
                        g_controller_raw[HAND_RIGHT].transform.pose[0],
                        g_controller_raw[HAND_RIGHT].transform.position[0],
                        g_controller_raw[HAND_RIGHT].transform.position[1],
                        g_controller_raw[HAND_RIGHT].transform.position[2],
                        getButtonPressedStr(g_controller_raw[HAND_RIGHT].button_mask,
                                            HAND_RIGHT));

                    int right_mask = right_controller->button_mask;
                    bool trigger_pressed = (right_mask & DPNP_ELITE_BUTTONMASK_TRIGER) != 0;
                    current_b_pressed = (right_mask & DPNP_ELITE_BUTTONMASK_YB) != 0;
                    bool b_pressed_edge = current_b_pressed && !previous_b_pressed;


                    float* cur_hand_pos_vr = g_controller_raw[HAND_RIGHT].transform.position;
                    float cur_hand_pos[3];
                    transformVRToModel(cur_hand_pos_vr, cur_hand_pos);

                    if (wait_trigger_release_after_reset && !trigger_pressed) {
                        wait_trigger_release_after_reset = false;
                        printf("[Trigger] Released after reset, ready to follow again.\n");
                    }

                    // 按下扳机：记录起始点
                    if (!wait_trigger_release_after_reset && trigger_pressed && !following) {
                        memcpy(start_hand_pos, cur_hand_pos, sizeof(start_hand_pos));
                        memcpy(start_model_pos, model_pos, sizeof(model_pos));

                        float* cur_hand_quat_vr = g_controller_raw[HAND_RIGHT].transform.pose;
                        transformQuatVRToModel(cur_hand_quat_vr, start_hand_quat_model);
                        memcpy(start_model_quat, model_quat, sizeof(model_quat));

                        following = true;
                        printf("\n[Trigger] PRESSED\n");
                        printf("  start_hand_pos = (%.4f, %.4f, %.4f)\n",
                               start_hand_pos[0], start_hand_pos[1], start_hand_pos[2]);
                        printf("  start_model_pos = (%.4f, %.4f, %.4f)\n",
                               start_model_pos[0], start_model_pos[1], start_model_pos[2]);
                        printf("  start_hand_quat_model = (%.4f, %.4f, %.4f, %.4f)\n",
                               start_hand_quat_model[0], start_hand_quat_model[1],
                               start_hand_quat_model[2], start_hand_quat_model[3]);
                        printf("  start_model_quat = (%.4f, %.4f, %.4f, %.4f)\n",
                               start_model_quat[0], start_model_quat[1],
                               start_model_quat[2], start_model_quat[3]);
                    }
                    // 跟随中：计算期望位置并钳位到安全球面
                    else if (!wait_trigger_release_after_reset && trigger_pressed && following) {
                        // 位置增量
                        float delta[3] = {
                            cur_hand_pos[0] - start_hand_pos[0],
                            cur_hand_pos[1] - start_hand_pos[1],
                            cur_hand_pos[2] - start_hand_pos[2]
                        };
                        float desired_pos[3] = {
                            start_model_pos[0] + change_rate * delta[0],
                            start_model_pos[1] + change_rate * delta[1],
                            start_model_pos[2] + change_rate * delta[2]
                        };

                        // 安全区处理：钳位到球面边缘
                        float dx = desired_pos[0] - init_model_pos[0];
                        float dy = desired_pos[1] - init_model_pos[1];
                        float dz = desired_pos[2] - init_model_pos[2];
                        float dist = sqrtf(dx*dx + dy*dy + dz*dz);
                        if (dist <= safe_radius) {
                            memcpy(model_pos, desired_pos, sizeof(model_pos));
                        } else {
                            // 超出安全区：投影到球面上
                            float scale = safe_radius / dist;
                            model_pos[0] = init_model_pos[0] + dx * scale;
                            model_pos[1] = init_model_pos[1] + dy * scale;
                            model_pos[2] = init_model_pos[2] + dz * scale;
                            printf("[Safety] Clamped to sphere: (%.4f,%.4f,%.4f) -> (%.4f,%.4f,%.4f)\n",
                                   desired_pos[0], desired_pos[1], desired_pos[2],
                                   model_pos[0], model_pos[1], model_pos[2]);
                        }

                        // 姿态更新
                        float* cur_hand_quat_vr = g_controller_raw[HAND_RIGHT].transform.pose;
                        float cur_hand_quat_model[4];
                        transformQuatVRToModel(cur_hand_quat_vr, cur_hand_quat_model);

                        float start_conj[4];
                        quatConjugate(start_hand_quat_model, start_conj);
                        float delta_quat[4];
                        quatMultiply(cur_hand_quat_model, start_conj, delta_quat);
                        quatNormalize(delta_quat);

                        float new_quat[4];
                        quatMultiply(delta_quat, start_model_quat, new_quat);
                        quatNormalize(new_quat);
                        memcpy(model_quat, new_quat, sizeof(model_quat));

                        // 发送最终位姿

                        float delta_norm = sqrtf(delta[0]*delta[0] + delta[1]*delta[1] + delta[2]*delta[2]);
                        printf("[Following] delta_pos=(%.4f, %.4f, %.4f) norm=%.6f, model_pos=(%.4f, %.4f, %.4f)\n",
                               delta[0], delta[1], delta[2], delta_norm,
                               model_pos[0], model_pos[1], model_pos[2]);
                        printf("  model_quat = (%.4f, %.4f, %.4f, %.4f)\n",
                                model_quat[0], model_quat[1], model_quat[2], model_quat[3]);
                    }
                    // 松开扳机：停止跟随
                    else if (!trigger_pressed && following) {
                        following = false;
                        printf("[Trigger] RELEASED, stop following\n");
                    }

//                  // ========== A 键：原有固定位姿复位 ==========
//                  if (a_pressed) {
//                      memcpy(model_pos, init_model_pos, sizeof(model_pos));
//                      memcpy(model_quat, init_model_quat, sizeof(model_quat));
//                        send_pose_to_arm(model_pos, model_quat);
//                        following = false;   // 退出跟随模式
//                        printf("[Reset-A] 固定位姿复位: pos=(%.4f,%.4f,%.4f) quat=(%.4f,%.4f,%.4f,%.4f)\n",
//                            model_pos[0], model_pos[1], model_pos[2],
//                           model_quat[0], model_quat[1], model_quat[2], model_quat[3]);
//                    }

                    // ========== B 键：通过服务端进行关键帧复位 ==========
                    if (b_pressed_edge) {
                        memcpy(model_pos, init_model_pos, sizeof(model_pos));
                        memcpy(model_quat, init_model_quat, sizeof(model_quat));
                        following = false;
                        wait_trigger_release_after_reset = trigger_pressed;
                        reset_robot_event = true;
                        recenter_vr_event = true;
                        printf("[B] Sent reset/recenter event.\n");
                        if (recenter()) {
                            printf("VR recenter success.\n");
                        } else {
                            printf("VR recenter failed.\n");
                        }
                    }


//                    if (grip_pressed) {
//                        recenter();
//                        printf("Grip button is pressed\n");
//                    }

                } else {
                    printf("right controller is disconnected\n\n\n\n");
                }
            } else {
                printf("HMD USB is not connected\n");
            }
            ArmControlFrame control_frame = build_arm_control_frame(
                model_pos,
                model_quat,
                tracking_ok,
                following,
                right_controller,
                reset_robot_event,
                recenter_vr_event);
            send_pose_to_arm(control_frame);
            previous_b_pressed = current_b_pressed;
            Sleep(50);
        }

    } while (0);

    // 清理资源
    if (g_instance) {
        DpnpCloseDevice(g_devices[DEVICE_HMD_INDEX]);
        DpnpCloseDevice(g_devices[DEVICE_CONTROLLER_LEFT_INDEX]);
        DpnpCloseDevice(g_devices[DEVICE_CONTROLLER_RIGHT_INDEX]);
        DpnpCloseDevice(g_devices[DEVICE_CONTROLLER_THIRD_INDEX]);
        DpnpCloseDevice(g_devices[DEVICE_STATION_MASTER_INDEX]);
        DpnpCloseDevice(g_devices[DEVICE_STATION_SLAVE_INDEX]);
        g_devices[DEVICE_HMD_INDEX] = NULL;
        g_devices[DEVICE_CONTROLLER_LEFT_INDEX] = NULL;
        g_devices[DEVICE_CONTROLLER_RIGHT_INDEX] = NULL;
        g_devices[DEVICE_CONTROLLER_THIRD_INDEX] = NULL;
        g_devices[DEVICE_STATION_MASTER_INDEX] = NULL;
        g_devices[DEVICE_STATION_SLAVE_INDEX] = NULL;
        DpnnDeinit(g_instance);
        g_instance = NULL;
    }
    return 0;
}

    
