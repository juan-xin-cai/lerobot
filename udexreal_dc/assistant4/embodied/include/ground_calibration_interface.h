#pragma once

#include "dpn_peripheral_interface_common.h"
#include "dpn_polaris_roomsetup_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** --------------------------------------------------------------------------------------------------------
Usage:
	第一个调用
Params:
	无
Return:
	true: 成功
	false: 失败
Remarks:
	调用 基站校准 DDK 的初始化函数
---------------------------------------------------------------------------------------------------------**/
DPN_API bool DpnpCalibrationInit();
typedef bool(*DpnpCalibrationInit_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
	最后一个调用，（中途）关闭程序的时候也要调用，以结束后台线程
Params:
	无
Return:
	无
Remarks:
	通知后台线程退出并等待其自动结束（就绪 -> 执行最多一次循环体的时间）,然后调用 基站校准 DDK 的 Deinit 函数
---------------------------------------------------------------------------------------------------------**/
DPN_API void DpnpCalibrationDeinit();
typedef void(*DpnpCalibrationDeinit_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
	启动后台Calibration线程
Params：
	height：Controller距离地面的高度，单位是米
	Controller 放在地面上，height 为 0；
	Controller 放在桌面上，用户输入桌面到地面的高度作为 height

	BaseIDUI: 主基站（1），从基站（2），双基站（3）
Return：
	DPNP_SETUP_RESULT_THREAD_EXISTS: background thread still/already exists
	DPNP_SETUP_RESULT_THREAD_RESOURCE_UNAVAILABLE_TRY_AGAIN: (system error) The thread could not be started
	DPNP_SETUP_RESULT_OK: create background thread and return
Remarks:
	本函数在启动后台线程后，立即返回，不会阻塞。
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupCalibrationBegin(float const height, int BaseIDUI);
typedef DPNP_SETUP_RESULT(*DpnpSetupCalibrationBegin_f)(float const height, int BaseIDUI);

/** --------------------------------------------------------------------------------------------------------
Usage:
	检查后台Calibration算法的当前状态
Params：
	无
Return：
	返回当前瞬间地面校准的状态，通常用来同步查看DpnpdSetupCalibrateGround()的状态
	DPNP_SETUP_RESULT_OK：状态正常。
	DPNP_SETUP_RESULT_DDK_CONDITION_CALIBRATION_IN_PROGRESS：正在校准地面，无异常。
	DPNP_SETUP_RESULT_DDK_CONDITION_WAIT_FOR_NEXT_INPUT：等待用户扣动扳机。
	DPNP_SETUP_RESULT_DDK_CONDITION_WAIT_FOR_TRIGGER_RELEASE：等待用户松开扳机。
	DPNP_SETUP_RESULT_NOT_INITIALIZED：未调用DpnpSetupInit()。
	DPNP_SETUP_RESULT_CALIBRATION_ABORT：用户调用DpnpdSetupCalibrationAbort()主动中止地面校准。
	DPNP_SETUP_RESULT_STATION_OFF：所有已经成功读取出场参数的基站无信号，可能的原因是基站电源关闭或者基站扫描范围内没有一个设备处于电源开启并可见状态。
	DPNP_SETUP_RESULT_DDK_DEVICE_CONNECTION_LOST：手柄连接丢失，可能的原因是基站电源关闭或者基站扫描范围内没有一个手柄处于电源开启并可见状态。
	DPNP_SETUP_RESULT_DDK_CONDITION_CONTROLLER_NOT_STABLE：用来校准地面的手柄不稳定，正在摇晃。
	DPNP_SETUP_RESULT_DDK_CONDITION_CONTROLLER_POSE_CHANGE：用来校准地面的手柄放置的姿态有较大变化。无法保证同一高度。
	DPNP_SETUP_RESULT_DDK_CONDITION_CONTROLLER_NO_ENOUGH_LIGHTS_SEEN：手柄无法被某一个基站看到足够多的接收器，通常是由于遮挡或者放置位置过偏。
	DPNP_SETUP_RESULT_DDK_CONDITION_CONTROLLER_CANNOT_BE_SEEN_TOGETHER：手柄不存在一个同时被主从基站看得到的接收器，通常是由于遮挡或者放置位置过偏。
	DPNP_SETUP_RESULT_DDK_CONDITION_CONTROLLER_NO_LIGHT_SEEN_IN_COMMON, // controller has no light been seen across CALIBRATION_DATA_COLLECTION_COUNT points for a same station. 采集失败。手柄摆放不同位置无法被同一基站看到同一盏灯，无法保证校准结果，抛弃此数据重新摆放并采集。
	DPNP_SETUP_RESULT_DDK_CONDITION_CONTROLLER_TOO_CLOSE, // Controller too close. 采集失败。手柄摆放位置与前几次摆放过于接近，无法保证校准结果，抛弃此数据重新摆放并采集。
	DPNP_SETUP_RESULT_DDK_CONDITION_CONTROLLER_CANNOT_COMPUTE_A,  //无法保证A基站至少在4个位置看到同一个灯
	DPNP_SETUP_RESULT_DDK_CONDITION_CONTROLLER_CANNOT_COMPUTE_B,  //无法保证B基站至少在4个位置看到同一个灯
	DPNP_SETUP_RESULT_DDK_CONDITION_CONTROLLER_CANNOT_COMPUTE_AB, //无法保证A、B基站至少在2个位置看到同一个灯
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupCheckCalibrationCondition();
typedef DPNP_SETUP_RESULT(*DpnpSetupCheckCalibrationCondition_f)();

DPN_API DPNP_SETUP_RESULT DpnpSetupGetCalibrationStage();
typedef DPNP_SETUP_RESULT(*DpnpSetupGetCalibrationStage_f)();

DPN_API void DpnpSetupSetCalibrationStage();
typedef void(*DpnpSetupSetCalibrationStage_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
	检查后台Calibration算法的当前状态
Params：
	无
Return：
	返回当前瞬间地面校准的状态，通常用来同步查看DpnpdSetupCalibrateGround()的状态
	DPNP_SETUP_RESULT_OK：状态正常。
	DPNP_SETUP_RESULT_DDK_CONDITION_CALIBRATION_IN_PROGRESS：正在校准地面，无异常。
	DPNP_SETUP_RESULT_DDK_CONDITION_WAIT_FOR_NEXT_INPUT：等待用户扣动扳机。
	DPNP_SETUP_RESULT_DDK_CONDITION_WAIT_FOR_TRIGGER_RELEASE：等待用户松开扳机。
	DPNP_SETUP_RESULT_NOT_INITIALIZED：未调用DpnpdSetupInit()。
	DPNP_SETUP_RESULT_CALIBRATION_ABORT：用户调用DpnpdSetupCalibrationAbort()主动中止地面校准。
	DPNP_SETUP_RESULT_DDK_DEVICE_HAND_CONNECTION_LOST：手中拿着的手柄连接丢失，可能的原因是基站电源关闭或者基站扫描范围内没有一个手柄处于电源开启并可见状态。
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupCheckCalibrationStatus();
typedef DPNP_SETUP_RESULT(*DpnpSetupCheckCalibrationStatus_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
    检查当前主基站是否支持单竖放模式
Params：
    无
Return：
    返回当前主基站是否支持单竖放模式
Remarks:
    在DpnpCalibrationInit之后, DpnpCalibrationDeinit之前调用
    true: 单基站竖放为可选状态; false: 应只有单基站校准模式和双基站校准模式可选
---------------------------------------------------------------------------------------------------------**/
DPN_API bool DpnpSetupCanSingleVertical();
typedef bool(*DpnpSetupCanSingleVertical_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
    设置当前主基站为单竖放模式
Params：
    无
Return：
    无
Remarks:
    在DpnpCalibrationInit之后, DpnpCalibrationDeinit之前调用
    用户选择单基站竖放模式后, 程序调用此函数进行设置, 该函数执行很快, 不阻塞
---------------------------------------------------------------------------------------------------------**/
DPN_API void DpnpSetupSetSingleVertical();
typedef void(*DpnpSetupSetSingleVertical_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
设置当前主基站为单横放模式
Params：
roller: 基站由竖放到横放，逆时针转为正90度，顺时针转为负90度；
pitch: 基站横放后，向下转动时转过的角度，如果不转为0，如果转了45度，则为45；
Return：
无
Remarks:
在DpnpCalibrationInit之后, DpnpCalibrationDeinit之前调用
用户选择单基站横放模式后, 程序调用此函数进行设置, 该函数执行很快, 不阻塞
---------------------------------------------------------------------------------------------------------**/
DPN_API void DpnpSetupSetSingleHorizonal(const float pitch, const float roller);
typedef void(*DpnpSetupSetSingleHorizonal_f)(const float pitch, const float roller);


/** --------------------------------------------------------------------------------------------------------
Usage:
	返回后台Calibration算法的当前进度，主基站已校准(?/4)个点
Params：
	无
Return：
	返回当前主基站已校准点的数目
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API int DpnpSetupGetMasterCalibratedNumber();
typedef int(*DpnpSetupGetMasterCalibratedNumber_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
	返回后台Calibration算法的当前进度，从基站已校准(?/4)个点
Params：
	无
Return：
	返回当前从基站已校准点的数目
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API int DpnpSetupGetSlaveCalibratedNumber();
typedef int(*DpnpSetupGetSlaveCalibratedNumber_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
	返回后台Calibration算法的当前进度，主从基站共同可见已校准(?/2)个点
Params：
	无
Return：
	返回当前主从基站共同可见已校准点的数目
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API int DpnpSetupGetDualCalibratedNumber();
typedef int(*DpnpSetupGetDualCalibratedNumber_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
	检测Calibration的进度
Params：
	out_progress: 返回Calibration的进度，0 ~ 100，100 代表已完成
Return：
	DPNP_SETUP_RESULT_PARAMETER_NULL_POINTER: out_progress == nullptr
	DPNP_SETUP_RESULT_CAUGHT_AN_EXCEPTION: if caught an exception
	DPNP_SETUP_RESULT_OK: 获取进度成功
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupCalibrationProgress(int* const out_progress);
typedef DPNP_SETUP_RESULT(*DpnpSetupCalibrationProgress_f)(int* const out_progress);

/** --------------------------------------------------------------------------------------------------------
Usage:
	通知Calibration线程退出
Params：
	无
Return：
	always return DPNP_SETUP_RESULT_OK
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupCalibrationAbort();
typedef DPNP_SETUP_RESULT(*DpnpSetupCalibrationAbort_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
	获得Calibration的结果
Params：
	无
Return：
	若 用户调用DpnpSetupCalibrationAbort()主动中止地面校准, 则返回DPNP_SETUP_RESULT_CALIBRATION_ABORT,
	否则:

	需要在 Calibration 进度 100%后，调用本函数检查：
	1. DPNP_SETUP_RESULT_NOT_INITIALIZED：未调用DpnpSetupInit()。
	2. DPNP_SETUP_RESULT_THREAD_CAUGHT_AN_EXCEPTION: 捕获到 DDK 执行 Calibration 期间的异常
	3. DPNP_SETUP_RESULT_OK: Calibration 成功
	4. DPNP_SETUP_RESULT_ROOM_INFO_BAD_SAVE: 保存地面的y坐标到文件时失败, 不影响steam流程的.
	5. DPNP_SETUP_RESULT_FAIL：请直接提示失败请重试(包含一种罕见的情况, 浮点计算出错)
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupCalibrationResult();
typedef DPNP_SETUP_RESULT(*DpnpSetupCalibrationResult_f)();

/** --------------------------------------------------------------------------------------------------------
Usage:
	判断基站校准精确度（非必须）
Params：
	无
Return：
	校准结果精确，或者是不精确
Remarks:
	DpnpSetupCalibrationProgress == 100 && DpnpSetupCalibrationResult == ( DPNP_SETUP_RESULT_OK||DPNP_SETUP_RESULT_ROOM_INFO_BAD_SAVE ) 之后调用
---------------------------------------------------------------------------------------------------------**/
DPN_API bool DpnpSetupCalibrationAccuracy();
typedef bool (*DpnpSetupCalibrationAccuracy_f)();

#ifdef __cplusplus
}
#endif
