/************************************************************************************
Filename    :
Content     :
Created     :   Oct 27, 2016
Authors     :   developer.deepoon.com
Copyright   :   Copyright 2016 Deepoon, Inc. All Rights reserved.
************************************************************************************/
#ifndef _DEEPOON_POLARIS_ROOM_SETUP_SDK_H_
#define _DEEPOON_POLARIS_ROOM_SETUP_SDK_H_

#include "dpn_peripheral_interface_common.h"
#include "dpn_polaris_roomsetup_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** --------------------------------------------------------------------------------------------------------
Usage:
	房间设置初始化
Params:
	无
Return:
	DPNP_SETUP_RESULT
Remarks:
---------------------------------------------------------------------------------------------------------**/

DPN_API DPNP_SETUP_RESULT DpnpRoomInit();
typedef DPNP_SETUP_RESULT(*DpnpRoomInit_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
	房间设置清理工作
Params:
	无
Return:
	无
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API void DpnpRoomDeinit();
typedef void (*DpnpRoomDeinit_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
	获取编号为station_index的基站所看到的type设备上的Mark数量
Params：
	station_index: 基站的编号
	type：设备类型
	out_count: station_index基站所看到的type设备上的Mark数量
Return：
	DPNP_SETUP_RESULT_CAUGHT_AN_EXCEPTION: if caught an exception
	DPNP_SETUP_RESULT_OK：成功。成功时可查看out_count指针的内容，表示station_index基站所看到的type设备上的Mark数量
	DPNP_SETUP_RESULT_NOT_INITIALIZED：未调用DpnpdSetupInit()。
	DPNP_SETUP_RESULT_PARAMETER_INVALID_STATION_INDEX：无效基站索引，目前只能使用0为基站A，1为基站B。
	DPNP_SETUP_RESULT_PARAMETER_NULL_POINTER：out_count为空指针。
	DPNP_SETUP_RESULT_PARAMETER_WRONG_DEVICE_TYPE：错误的设备类型。目前只支持DPNP_DEVICE_TYPE_HEAD_TRACKER、DPNP_DEVICE_TYPE_LEFT_CONTROLLER以及DPNP_DEVICE_TYPE_RIGHT_CONTROLLER
	DPNP_SETUP_RESULT_STATION_OFF：当前基站无信号，可能的原因是基站电源关闭或者基站扫描范围内没有一个设备处于电源开启并可见状态。
	DPNP_SETUP_RESULT_DDK_DEVICE_CONNECTION_LOST：当前设备连接失败。可能的原因是设备未开启电源、未连接到电脑或者不在任一基站可见范围内。
	DPNP_SETUP_RESULT_DDK_DEVICE_OUT_OF_SIGHT：当前设备已连接却不在当前基站可见范围内。
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupGetVisibleMarksCount(int const station_index, DPNP_DEVICE_TYPE const type, int* const out_count);
typedef DPNP_SETUP_RESULT (*DpnpSetupGetVisibleMarksCount_f)(int const station_index, DPNP_DEVICE_TYPE const type, int* const out_count);

/** --------------------------------------------------------------------------------------------------------
Usage:
	将扳机当前的状态值转换成bool值
Params：
	type：Device type，为Left Controller或者Right Controller
	out_Is_Pressed：转换成的bool值
Return：
	DPNP_SETUP_RESULT_CAUGHT_AN_EXCEPTION: if caught an exception
	DPNP_SETUP_RESULT_OK：成功。成功时可查看out_axis指针的内容，表示当前手柄的扳机状态
	DPNP_SETUP_RESULT_NOT_INITIALIZED：未调用DpnpdSetupInit()。
	DPNP_SETUP_RESULT_PARAMETER_NULL_POINTER：out_axis为空指针。
	DPNP_SETUP_RESULT_PARAMETER_WRONG_DEVICE_TYPE：错误的设备类型。目前只支持DPNP_DEVICE_TYPE_LEFT_CONTROLLER和DPNP_DEVICE_TYPE_RIGHT_CONTROLLER
	DPNP_SETUP_RESULT_DDK_DEVICE_CONNECTION_LOST：当前设备连接失败。可能的原因是设备未开启电源或者不在任一基站可见范围内。
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupIsTriggerPressed(DPNP_DEVICE_TYPE const type, bool* const out_Is_Pressed);
typedef DPNP_SETUP_RESULT (*DpnpSetupIsTriggerPressed_f)(DPNP_DEVICE_TYPE const type, bool* const out_Is_Pressed);

/** --------------------------------------------------------------------------------------------------------
Usage:
	启动数据采集线程，采集的数据用于指定Room Scale模式房间坐标系的-Z轴大致方向
Params：
	type：Device type，为Left Controller或者Right Controller
Return：
	DPNP_SETUP_RESULT_PARAMETER_WRONG_DEVICE_TYPE: type is not Left controller and is not right controller
	DPNP_SETUP_RESULT_THREAD_EXISTS: background thread still/already exists
	DPNP_SETUP_RESULT_THREAD_RESOURCE_UNAVAILABLE_TRY_AGAIN: (system error) The thread could not be started
	DPNP_SETUP_RESULT_OK: create background thread and return
Remarks:
	本函数在启动后台线程后，立即返回，不会阻塞
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupDirectionBegin(DPNP_DEVICE_TYPE const type);
typedef DPNP_SETUP_RESULT (*DpnpSetupDirectionBegin_f)(DPNP_DEVICE_TYPE const type);

/** --------------------------------------------------------------------------------------------------------
Usage:
	检查数据采集的状态
Params:
	无
Return:
	DPNP_SETUP_RESULT, 参见 DpnpSetupIsTriggerPressed
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupDirectionStatus();
typedef DPNP_SETUP_RESULT (*DpnpSetupDirectionStatus_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
	检查数据采集的进度
Params：
	out_progress: 返回当前的进度，0 ~ 100，返回100时表示结束
Return：
	DPNP_SETUP_RESULT_PARAMETER_NULL_POINTER: out_progress == nullptr，获取进度失败
	DPNP_SETUP_RESULT_OK: 获取进度成功
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupDirectionProgress(int* const out_progress);
typedef DPNP_SETUP_RESULT (*DpnpSetupDirectionProgress_f)(int* const out_progress);

/** --------------------------------------------------------------------------------------------------------
Usage:
	通知数据采集线程退出
Params：
	无
Return：
	always return DPNP_SETUP_RESULT_OK
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupDirectionAbort();
typedef DPNP_SETUP_RESULT (*DpnpSetupDirectionAbort_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
	获得type设备在定位坐标系(fix to station(s), Y轴竖直向上的右手系)中的位置
Params：
	type：设备类型。
	out_position: 传出type设备的在定位坐标系中的位置坐标。
Return：
	DPNP_SETUP_RESULT_OK：成功。成功时可查看out_position指针的内容，表示当前type手柄的位置
	DPNP_SETUP_RESULT_NOT_INITIALIZED：未调用DpnpdSetupInit()。
	DPNP_SETUP_RESULT_PARAMETER_NULL_POINTER：out_position为空指针。
	DPNP_SETUP_RESULT_PARAMETER_WRONG_DEVICE_TYPE：错误的设备类型。目前只支持DPNP_DEVICE_TYPE_LEFT_CONTROLLER和DPNP_DEVICE_TYPE_RIGHT_CONTROLLER
	DPNP_SETUP_RESULT_DDK_DEVICE_CONNECTION_LOST：当前设备连接失败。可能的原因是设备未开启电源或者不在任一基站可见范围内。
	DPNP_SETUP_RESULT_DDK_STATIONS_NOT_CALIBRATED, // Stations are not calibrated. 基站尚未校准。
Remarks:
	out_position[0]=x, out_position[1]=y, out_position[2]=z, (x,y,z)是type设备在定位坐标系中的坐标
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupGetPositionInertia(DPNP_DEVICE_TYPE const type, float out_position[3]);
typedef DPNP_SETUP_RESULT (*DpnpSetupGetPositionInertia_f)(DPNP_DEVICE_TYPE const type, float out_position[3]);
// DPN_API DPNP_SETUP_RESULT DpnpSetupGetPositionInertia_DataBuffer(DPNP_DEVICE_TYPE const type, float* const out_position, int* const inout_size);

/** --------------------------------------------------------------------------------------------------------
Usage:
	传入一个空间四边形的四个顶点在定位坐标系中的坐标，传出其地面投影的最大内接矩形（的四个顶点在定位坐标系中的坐标），并传出房间坐标系的Z轴方向。
Params：
	in_corner：四边形的4个顶点坐标（顺时针 或者 逆时针）
	out_rect：传出矩形的4个顶点坐标。
	out_z_axis：传出房间坐标系Z轴的方向。
Return：
	DPNP_SETUP_RESULT_OK: success
	DPNP_SETUP_RESULT_NONCONVEX: in_corner is not a simple convex polygon
	DPNP_SETUP_RESULT_SPACE_TOO_SMALL: the rectangle is smaller than 1m*1m
	DPNP_SETUP_RESULT_ROOM_INFO_BAD_SAVE：failed to open/create a file to save information
Remarks:
---------------------------------------------------------------------------------------------------------**/
typedef float dpn_point_xyz[3]; // dpn_point_xyz p; p[0] = x; p[1] = y; p[2] = z;
DPN_API DPNP_SETUP_RESULT DpnpSetupRectangle(const dpn_point_xyz in_corner[4], dpn_point_xyz out_rect[4], dpn_point_xyz out_z_axis);
typedef DPNP_SETUP_RESULT (*DpnpSetupRectangle_f)(const dpn_point_xyz in_corner[4], dpn_point_xyz out_rect[4], dpn_point_xyz out_z_axis);

/** --------------------------------------------------------------------------------------------------------
Usage:
	启动一个后台线程，确定Stand Only模式下房间坐标系的朝向和原点的位置
Params：
	type：device type
Return：
	DPNP_SETUP_RESULT_PARAMETER_WRONG_DEVICE_TYPE: type is not Left controller and is not right controller
	DPNP_SETUP_RESULT_THREAD_EXISTS: background thread still/already exists
	DPNP_SETUP_RESULT_THREAD_RESOURCE_UNAVAILABLE_TRY_AGAIN: (system error) The thread could not be started
	DPNP_SETUP_RESULT_OK: create background thread and return
Remarks:
	本函数在启动后台线程后，立即返回，不会阻塞
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupCoordinatesBegin(DPNP_DEVICE_TYPE const type);
typedef DPNP_SETUP_RESULT (*DpnpSetupCoordinatesBegin_f)(DPNP_DEVICE_TYPE const type);

/** --------------------------------------------------------------------------------------------------------
Usage:
	检查状态
Params:
	无
Return:
	DPNP_SETUP_RESULT, 参见 DpnpSetupIsTriggerPressed
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupCoordinatesStatus();
typedef DPNP_SETUP_RESULT (*DpnpSetupCoordinatesStatus_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
	获取设置Stand Only模式下房间坐标系的进度
Params：
	out_progress：传出进度，0~100，100表示已完成
Return：
	DPNP_SETUP_RESULT_PARAMETER_NULL_POINTER: out_progress == nullptr，获取进度失败
	DPNP_SETUP_RESULT_OK: 获取进度成功
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupCoordinatesProgress(int* const out_progress);
typedef DPNP_SETUP_RESULT (*DpnpSetupCoordinatesProgress_f)(int* const out_progress);

/** --------------------------------------------------------------------------------------------------------
Usage:
	通知后台线程退出
Params：
	无
Return：
	always return DPNP_SETUP_RESULT_OK
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupCoordinatesAbort();
typedef DPNP_SETUP_RESULT (*DpnpSetupCoordinatesAbort_f)();

#ifdef __cplusplus
}
#endif

#endif /* _DEEPOON_POLARIS_ROOM_SETUP_SDK_H_ */
