#pragma once
#include <vector>
#include "DpnpCommonEventParam.h"


enum class DpvrRoomSetupMode
{
    Standard = 0,
    Headset = 1,
    VioOnly = 2,
    Simple = 3,
};


struct DpvrHeadsetModeParam
{
    float headset_height = 1.0f;
    float headset_gamearea = 1.5f;
    int hide_fence = 0;
    int simulator_mode = 0;
    int controller_index = 0;
    int controller_pose = 0;
    float controller_height = 1.0f;
    float controller_depth = 0.5f;
};


struct DpvrStandardModeParam
{

};

struct DpvrSimpleModeParam
{
    std::vector<int> heights; // in cm
    int current_height_index;
    int radius; //in cm
};


struct DpvrRoomSetupParam
{
    DpvrRoomSetupMode roomsetupMode = {};

    DpvrStandardModeParam standard = {};
    DpvrHeadsetModeParam headset;
    DpvrSimpleModeParam simple;

    DpvrRoomSetupParam() {}

    DpvrRoomSetupParam(const DpvrRoomSetupParam& other)
    {
        this->roomsetupMode = other.roomsetupMode;

        switch (roomsetupMode)
        {
        case DpvrRoomSetupMode::Standard:
            this->standard = other.standard;
            break;
        case DpvrRoomSetupMode::Headset:
            this->headset = other.headset;
            break;
        case DpvrRoomSetupMode::VioOnly:
            this->headset = other.headset;
            break;
        case DpvrRoomSetupMode::Simple:
            this->simple = other.simple;
            break;
        default:
            break;
        }
    }

    ~DpvrRoomSetupParam() {}

    void Clear()
    {
        *this = {};
    }

    int Serialize(void* buf, int offset, int bufLen) const
    {
        int len = 0;
        len = DpnpCommonEventParam::Serialize(roomsetupMode, buf, offset, bufLen); offset += len;
        switch (roomsetupMode)
        {
        case DpvrRoomSetupMode::Standard:
            break;
        case DpvrRoomSetupMode::Headset:
        case DpvrRoomSetupMode::VioOnly:
            len = DpnpCommonEventParam::Serialize(headset.headset_height, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Serialize(headset.headset_gamearea, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Serialize(headset.hide_fence, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Serialize(headset.simulator_mode, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Serialize(headset.controller_index, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Serialize(headset.controller_pose, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Serialize(headset.controller_height, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Serialize(headset.controller_depth, buf, offset, bufLen); offset += len;
            break;
        case DpvrRoomSetupMode::Simple:
            len = DpnpCommonEventParam::Serialize(simple.heights, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Serialize(simple.current_height_index, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Serialize(simple.radius, buf, offset, bufLen); offset += len;
            break;
        default:
            break;
        }
        return len;
    }

    int Deserialize(void* buf, int offset, int bufLen)
    {
        int len = 0;
        len = DpnpCommonEventParam::Deserialize(roomsetupMode, buf, offset, bufLen); offset += len;
        switch (roomsetupMode)
        {
        case DpvrRoomSetupMode::Standard:
            break;
        case DpvrRoomSetupMode::Headset:
        case DpvrRoomSetupMode::VioOnly:
            len = DpnpCommonEventParam::Deserialize(headset.headset_height, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Deserialize(headset.headset_gamearea, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Deserialize(headset.hide_fence, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Deserialize(headset.simulator_mode, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Deserialize(headset.controller_index, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Deserialize(headset.controller_pose, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Deserialize(headset.controller_height, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Deserialize(headset.controller_depth, buf, offset, bufLen); offset += len;
            break;
        case DpvrRoomSetupMode::Simple:
            len = DpnpCommonEventParam::Deserialize(simple.heights, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Deserialize(simple.current_height_index, buf, offset, bufLen); offset += len;
            len = DpnpCommonEventParam::Deserialize(simple.radius, buf, offset, bufLen); offset += len;
            break;
        default:
            break;
        }
        return len;
    }
};

