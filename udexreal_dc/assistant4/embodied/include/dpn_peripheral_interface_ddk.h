/******************************************************************************
Filename    :
Content     :
Created     :   July, 2016
Authors     :   deepoon.com
Copyright   :   Copyright 2016 Deepoon, Inc. All Rights reserved.
*******************************************************************************/
#ifndef DPN_PERIPHERAL_INTERFACE_DDK_H
#define DPN_PERIPHERAL_INTERFACE_DDK_H

#include "dpn_peripheral_interface_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void * DPNPD_INSTANCE;
typedef void * DPNPD_DEVICE_ENUMERATOR;
typedef void * DPNPD_DEVICE;

typedef DPNP_VALUE_TYPE DPNPD_VALUE_TYPE;
typedef DPNP_EVENT_TYPE DPNPD_EVENT_TYPE;
typedef DPNP_DEVICE_TYPE DPNPD_DEVICE_TYPE;

typedef void(*DpnpdHandleEventFunc)(DPNPD_DEVICE device, int event_mask, void *user_data);

enum 
{
	DPVR_AUTOCALIB_RESULT_OK = 0,//校准完成
	DPVR_AUTOCALIB_RESULT_FAIL,//校准失败
	DPVR_AUTOCALIB_RESULT_INPROCESS,//校准中
	DPVR_AUTOCALIB_RESULT_MASTER_FINISHED,//主基站校准完成
	DPVR_AUTOCALIB_RESULT_SLAVE_FINISHED,//从基站校准完成
	DPVR_AUTOCALIB_RESULT_DUAL_BASE_FINISHED,//双基站校准完成
	DPVR_AUTOCALIB_RESULT_MASTER_PARAMETER_FAIL,//获取主基站校准参数失败，头盔靠近主基站一些
	DPVR_AUTOCALIB_RESULT_SLAVE_PARAMETER_FAIL,//获取从基站校准参数失败，头盔靠近从基站一些
	DPVR_AUTOCALIB_RESULT_DUAL_BASE_PARAMETER_FAIL,//获取双基站校准参数失败，检查头盔和基站间的RF通信
	DPVR_AUTOCALIB_RESULT_BASE_POSITION_ERROR_TOO_LARGE,//校准失败：校准偏差较大，请将头盔放置在两个基站中间
	DPVR_AUTOCALIB_RESULT_ERROR_TOO_LARGE,//校准失败：校准偏差太大，请调整双基站的位置，双基站最好相对放置
	DPVR_AUTOCALIB_RESULT_MASTER_LIGHT_NOT_ENOUGH,//主基站未照到两个小灯
	DPVR_AUTOCALIB_RESULT_SLAVE_LIGHT_NOT_ENOUGH,//主基站未照到两个小灯


	DPVR_AUTOCALIB_RESULT_NUM
};

typedef int DPVR_AUTOCALIB_RESULT; // Polaris1.1自动校准过程中的校准结果

typedef struct
{
	char deviceName[DPNP_STR_LEN_SHORT];
	DPNPD_DEVICE_TYPE type;
} dpnpdDeviceInfo;

#define DPNPD_MAKE_VERSION(major, minor, rev) ((major&0xff)|((minor&0xff)<<8)|(rev<<16))

//-----------------------------------------------------------------
// peripheral interface version information
//-----------------------------------------------------------------
DPN_API	int DpnpdGetVersion();
DPN_API	int DpnpdGetRuntimeVersion();

//-----------------------------------------------------------------
// peripheral interface initialization
//-----------------------------------------------------------------
DPN_API	DPNPD_INSTANCE DpnpdInit(void* activity);
DPN_API	DPNPD_INSTANCE DpnpdInit2(void* activity,int APIVersion);
DPN_API	bool DpnpdDeinit(DPNPD_INSTANCE instance);

//-----------------------------------------------------------------
// peripheral device enumerator
//-----------------------------------------------------------------
DPN_API	DPNPD_DEVICE_ENUMERATOR DpnpdGetDeviceEnumerator(DPNPD_INSTANCE instance);
DPN_API	bool DpnpdGetNextDevice(DPNPD_DEVICE_ENUMERATOR enumerator, dpnpdDeviceInfo* info);
DPN_API	bool DpnpdRestartEnumerator(DPNPD_DEVICE_ENUMERATOR enumerator);
DPN_API	bool DpnpdCloseEnumerator(DPNPD_DEVICE_ENUMERATOR enumerator);

//-----------------------------------------------------------------
// peripheral device io
//-----------------------------------------------------------------
DPN_API	DPNPD_DEVICE DpnpdOpenDevice(DPNPD_INSTANCE instance, const char *deviceName);
DPN_API	bool DpnpdCloseDevice(DPNPD_DEVICE device);

DPN_API	bool DpnpdGetInt(DPNPD_DEVICE device, DPNPD_VALUE_TYPE type, int key, int *pValue, int count);
DPN_API	bool DpnpdPutInt(DPNPD_DEVICE device, DPNPD_VALUE_TYPE type, int key, int *pValue, int count);

DPN_API	bool DpnpdGetFloat(DPNPD_DEVICE device, DPNPD_VALUE_TYPE type, int key, float *pValue, int count);
DPN_API	bool DpnpdPutFloat(DPNPD_DEVICE device, DPNPD_VALUE_TYPE type, int key, float *pValue, int count);

DPN_API	bool DpnpdGetDouble(DPNPD_DEVICE device, DPNPD_VALUE_TYPE type, int key, double *pValue, int count);
DPN_API	bool DpnpdPutDouble(DPNPD_DEVICE device, DPNPD_VALUE_TYPE type, int key, double *pValue, int count);

DPN_API	int  DpnpdGetVoid(DPNPD_DEVICE device, DPNPD_VALUE_TYPE type, int key, void *pValue, int count);
DPN_API	bool DpnpdPutVoid(DPNPD_DEVICE device, DPNPD_VALUE_TYPE type, int key, void *pValue, int count);

DPN_API void DpnpdResume();
DPN_API void DpnpdPause();

DPN_API bool DpnpdRegisterEventNotificationFunction(DPNPD_DEVICE device, DpnpdHandleEventFunc callback, void *userData);

#ifdef __cplusplus
}
#endif

#endif //DPN_PERIPHERAL_INTERFACE_DDK_H
