#pragma once

#include "dpn_peripheral_interface_common.h"
#include "dpn_polaris_roomsetup_common.h"

/** --------------------------------------------------------------------------------------------------------
Usage:
	清除基站参数
Params:
	index: 基站的编号。0为基站A，1为基站B
Return:
	DPNP_SETUP_RESULT_OK：只要传入的参数OK, 就一定成功。
	DPNP_SETUP_RESULT_PARAMETER_INVALID_STATION_INDEX：无效基站索引，目前只能使用0为基站A，1为基站B。
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupClearStationParameter(int const index);
typedef DPNP_SETUP_RESULT(*DpnpSetupClearStationParameter_f)(int const index);

/** --------------------------------------------------------------------------------------------------------
Usage:
	获取基站出厂参数。此函数调用时必须连接基站，并将拨码开关按操作提示拨到正确位置。
Params:
	index: 基站的编号。0为基站A，1为基站B
Return:
	DPNP_SETUP_RESULT_OK：成功。
	DPNP_SETUP_RESULT_NOT_INITIALIZED：未调用DpnpSetupInit()。
	DPNP_SETUP_RESULT_PARAMETER_INVALID_STATION_INDEX：无效基站索引，目前只能使用0为基站A，1为基站B。
	DPNP_SETUP_RESULT_DDK_DEVICE_CONNECTION_LOST：基站USB连接失败，通常原因是基站未上电或者基站未连接USB。
	DPNP_SETUP_RESULT_DDK_WRONG_STATION_ROLE：当前连接基站角色错误。通常因为拨码开关位置错误。
	DPNP_SETUP_RESULT_DDK_INVALID_DATA：无效数据。当前基站内数据无效，可能由于序列号、校准数据等错误。如果多次遇见此错误请联系DPVR。
	DPNP_SETUP_RESULT_DDK_STATION_SN_DUPLICATED：序列号重复。设置基站B时发现和基站A的序列号相同。用户需更换另一台基站进行设置。
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupReadStationParameter(int const index);
typedef DPNP_SETUP_RESULT(*DpnpSetupReadStationParameter_f)(int const index);

/** --------------------------------------------------------------------------------------------------------
Usage:
	获取已读取参数的基站序列号，此函数可用来判断A、B基站是否已读取出厂参数。此函数调用时无需连接基站。
Params:
	index: 基站的编号。0为基站A，1为基站B
	sn: 外部分配的内存空间首地址，请确保至少为16字节
Return:
	DPNP_SETUP_RESULT_OK：成功。成功时可查看sn指针的内容，如果已读取出厂参数则sn返回相应基站序列号，否则sn[0] == '\0'
	DPNP_SETUP_RESULT_NOT_INITIALIZED：未调用DpnpSetupInit()。
	DPNP_SETUP_RESULT_PARAMETER_INVALID_STATION_INDEX：无效基站索引，目前只能使用0为基站A，1为基站B。
	DPNP_SETUP_RESULT_PARAMETER_NULL_POINTER：sn为空指针。
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupGetStationSerialNumber(int const index, char* sn);
typedef DPNP_SETUP_RESULT(*DpnpSetupGetStationSerialNumber_f)(int const index, char* sn);

/** --------------------------------------------------------------------------------------------------------
Usage:
	获取当前支持的基站数量
Params:
	out_count: 返回当前支持的基站数量
Return:
	DPNP_SETUP_RESULT_FAIL: out_count == nullptr
	DPNP_SETUP_RESULT_CAUGHT_AN_EXCEPTION: if caught an exception
	DPNP_SETUP_RESULT_OK: success
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupGetSupportedStationCount(int* const out_count);
typedef DPNP_SETUP_RESULT (*DpnpSetupGetSupportedStationCount_f)(int* const out_count);

/** --------------------------------------------------------------------------------------------------------
Usage:
	获取基站的状态
Params:
	index: 基站的编号。0为基站A，1为基站B
Return:
	DPNP_SETUP_RESULT_CAUGHT_AN_EXCEPTION: if caught an exception
	DPNP_SETUP_RESULT_NOT_INITIALIZED：未调用DpnpdSetupInit()。
	DPNP_SETUP_RESULT_PARAMETER_INVALID_STATION_INDEX：无效基站索引，目前只能使用0为基站A，1为基站B。
	DPNP_SETUP_RESULT_STATION_OFF：当前基站无信号，可能的原因是基站电源关闭或者基站扫描范围内没有一个设备处于电源开启并可见状态。
	DPNP_SETUP_RESULT_STATION_ON：当前基站为开启状态。
	DPNP_SETUP_RESULT_STATION_CALIBRATED：当前基站已经地面校准。
Remarks:
---------------------------------------------------------------------------------------------------------**/
DPN_API DPNP_SETUP_RESULT DpnpSetupGetStationStatus(int const index);
typedef DPNP_SETUP_RESULT (*DpnpSetupGetStationStatus_f)(int const index);
