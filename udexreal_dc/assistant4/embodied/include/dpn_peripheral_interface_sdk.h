/******************************************************************************
Filename    :
Content     :
Created     :   July, 2016
Authors     :   deepoon.com
Copyright   :   Copyright 2016 Deepoon, Inc. All Rights reserved.
*******************************************************************************/
#ifndef DPN_PERIPHERAL_INTERFACE_SDK_H
#define DPN_PERIPHERAL_INTERFACE_SDK_H

#include "dpn_peripheral_interface_common.h"

typedef void(*DpnpReadPoseFunc)(DPNP_DEVICE device, void *user_data, float pose[DPNP_VALUE_TYPE_SIZE_POSE]);
typedef void(*DpnpReadPositionFunc)(DPNP_DEVICE device, void *user_data, float position[DPNP_VALUE_TYPE_SIZE_POSITION]);
typedef void(*DpnpHandleEventFunc)(DPNP_DEVICE device, int event_mask, void *user_data);

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CONTROLLER_COUNT 2
#pragma pack( 1 )
typedef struct
{
    float s;
    float i;
    float j;
    float k;
} DPN_SDK_QUARTERION;

typedef struct
{
    float x;
    float y;
    float z;
} DPN_SDK_VECTOR3;

typedef struct
{
    float x;
    float y;
    float w;
    float h;
} DPN_SDK_RECT;

typedef struct
{
    float m[4][4];
    float l, r, t, b;
}DPN_SDK_MATRIX;

#pragma pack()

typedef struct _DpnnDevicePose
{
    DPN_SDK_QUARTERION  rotation;
    DPN_SDK_VECTOR3     angular_velocity;
    DPN_SDK_VECTOR3     angular_acceleration;
} DpnnDevicePose;
typedef struct _DpnnDevicePosition
{
    DPN_SDK_VECTOR3     position;
    DPN_SDK_VECTOR3     position_velocity;
    DPN_SDK_VECTOR3     position_acceleration;
} DpnnDevicePosition;
typedef struct _DpnnDeviceTransform
{
    dpnpTransform       transform;
    int                 is_valid;
    int                 is_connected;
} DpnnDeviceTransform;
typedef struct _DpnnControllerState
{
    unsigned int        packet_number;
    float               touchpad_analog_x;
    float               touchpad_analog_y;
    float               joystick_analog_x;
    float               joystick_analog_y;
    float               trigger_analog;
    float               grip_analog;
    float               battery_analog;
    dpnpbutton_t        button_clicked_flags;
    dpnpbutton_t        button_touched_flags;
    int                 is_valid;

    _DpnnControllerState()
    {
        packet_number = 0;
        touchpad_analog_x = 0;
        touchpad_analog_y = 0;
        joystick_analog_x = 0;
        joystick_analog_y = 0;
        trigger_analog = 0;
        grip_analog = 0;
        battery_analog = 0;
        button_clicked_flags = 0;
        button_touched_flags = 0;
        is_valid = 0;
    }
} DpnnControllerState;
typedef struct _DpnnControllerRecord
{
    DpnnDeviceTransform     controllerTransform;
    DpnnControllerState     controllerState;
} DpnnControllerRecord;
typedef struct _DpnnControllerRecordOriginal
{
    dpnpTransform           transform;
    int                     connect_state;
    int                     button_mask;
    int                     touch_mask;
    float                   analog[DPNP_VALUE_TYPE_SIZE_ANALOG];
} DpnnControllerRecordOriginal;

// query device count
DPN_API int DpnpQueryDeviceCount(DPNP_DEVICE_TYPE type);
// get device id
DPN_API const char * DpnpGetDeviceId(DPNP_DEVICE_TYPE type, int index);
// open device
DPN_API DPNP_DEVICE DpnpOpenDevice(const char* device_id);
// pause device
DPN_API void DpnpPause(DPNP_DEVICE device);
// resume device
DPN_API void DpnpResume(DPNP_DEVICE device);
// close device
DPN_API void DpnpCloseDevice(DPNP_DEVICE device);
// get device transform count
DPN_API int DpnpGetDeviceTransformCount(DPNP_DEVICE device);
// get device pose count
DPN_API int DpnpGetDevicePoseCount(DPNP_DEVICE device);
// get device position count
DPN_API int DpnpGetDevicePositionCount(DPNP_DEVICE device);
// get device time count
DPN_API int DpnpGetDeviceTimeCount(DPNP_DEVICE device);
// get device quaternion count
DPN_API int DpnpGetDeviceQuaternionCount(DPNP_DEVICE device);
// get device vector count
DPN_API int DpnpGetDeviceVectorCount(DPNP_DEVICE device);
// get device axis count
DPN_API int DpnpGetDeviceAxisCount(DPNP_DEVICE device);
// get device analog count
DPN_API int DpnpGetDeviceAnalogCount(DPNP_DEVICE device);
// get device button count
DPN_API int DpnpGetDeviceButtonCount(DPNP_DEVICE device);
// get device attribute count
DPN_API int DpnpGetDeviceAttributeCount(DPNP_DEVICE device);
// get device feedback motor count
DPN_API int DpnpGetDeviceFeedbackCount(DPNP_DEVICE device);
// read device id
DPN_API const char * DpnpReadDeviceId(DPNP_DEVICE device);
// get associated device. e.g. use left hand device to get right hand device, etc
DPN_API const char * DpnpGetAssociatedDevice(DPNP_DEVICE device, DPNP_DEVICE_TYPE type);
// read device transform
DPN_API bool DpnpReadDeviceTransform(DPNP_DEVICE device, int index, dpnpTransform* transform);
// read device pose
DPN_API bool DpnpReadDevicePose(DPNP_DEVICE device, int index, float pose[DPNP_VALUE_TYPE_SIZE_POSE]);
// read device position
DPN_API bool DpnpReadDevicePosition(DPNP_DEVICE device, int index, float position[DPNP_VALUE_TYPE_SIZE_POSITION]);
// read device time
DPN_API double DpnpReadDeviceTime(DPNP_DEVICE device, int index);
// read device quaternion
DPN_API bool DpnpReadDeviceQuaternion(DPNP_DEVICE device, int index, float quaternion[DPNP_VALUE_TYPE_SIZE_QUATERNION]);
// read device vector
DPN_API bool DpnpReadDeviceVector(DPNP_DEVICE device, int index, float vector[DPNP_VALUE_TYPE_SIZE_VECTOR]);
// read device axis
DPN_API float DpnpReadDeviceAxis(DPNP_DEVICE device, int index);
// read device analog
DPN_API bool DpnpReadDeviceAnalog(DPNP_DEVICE device, int index, float analog[DPNP_VALUE_TYPE_SIZE_ANALOG]);
// read device button
DPN_API int DpnpReadDeviceButton(DPNP_DEVICE device, int index);
// read device attribute
DPN_API int DpnpReadDeviceAttribute(DPNP_DEVICE device, int index, void * buffer, int buffer_length);
// set device attribute
DPN_API bool DpnpSetDeviceAttribute(DPNP_DEVICE device, int index, void * buffer, int buffer_length);
// reset device pose
DPN_API void DpnpResetDevicePose(DPNP_DEVICE device, int index, float pose[DPNP_VALUE_TYPE_SIZE_POSE]);
// reset device position
DPN_API void DpnpResetDevicePosition(DPNP_DEVICE device, int index, float position[DPNP_VALUE_TYPE_SIZE_POSITION]);
// Register reference pose function -- used for drift correction
DPN_API void DpnpRegisterReferencePoseFunction(DPNP_DEVICE device, int index, DpnpReadPoseFunc callback, void *user_data);
// register reference position function -- used for drift correction
DPN_API void DpnpRegisterRererencePositionFunction(DPNP_DEVICE device, int index, DpnpReadPositionFunc callback, void *user_data);
// register event callback function
DPN_API void DpnpRegisterEventNotificationFunction(DPNP_DEVICE device, DpnpHandleEventFunc callback, int event_mask, void *user_data);
// write feedback to device
DPN_API void DpnpWriteDeviceFeedback(DPNP_DEVICE device, int index, float value);

DPN_API void DpnpHandleControllerData(DPNP_DEVICE device, int hand, DpnnControllerRecordOriginal* src, DpnnControllerRecord* dest);

DPN_API bool DpnpRefreshGpuConnectStatus(DPNP_DEVICE device);

#ifdef __cplusplus
}
#endif

#endif //DPN_PERIPHERAL_INTERFACE_SDK_H
