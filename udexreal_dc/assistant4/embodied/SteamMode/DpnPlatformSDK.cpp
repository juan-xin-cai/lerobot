/************************************************************************************
Filename    :
Content     :
Created     :   Feb , 2016
Authors     :   Tiehan Lu
Copyright   :   Copyright 2016 Deepoon, Inc. All Rights reserved.
************************************************************************************/
#include "stdafx.h"

#include <deepoon_sdk_native.h>
#include "dpn_peripheral_interface_sdk.h"

#include <stdio.h>
#include <string.h>
#include <thread>

#ifdef WIN32
#include <Windows.h>
#include <tchar.h>
#else
#include <dlfcn.h>
#include <math.h>
#include <unistd.h>
typedef void* HINSTANCE;
typedef unsigned long DWORD;
typedef bool BOOL;
#define FALSE false
#define TRUE true
#define OutputDebugString(x) printf(x)
#define OutputDebugStringA(x) printf(x)
#define _T(x) x

static void Sleep(int ms) {
  usleep(ms * 1000);
}

static HINSTANCE LoadLibrary(const char* lib_path) {
  if (lib_path == NULL || strlen(lib_path) == 0) {
    fprintf(stderr, "请检查库文件路径\n");
    return nullptr;
  }

  HINSTANCE hModule = dlopen(lib_path, RTLD_NOW | RTLD_GLOBAL);
  if (hModule == NULL) {
    fprintf(stderr, "加载库失败: %s\n", dlerror());
  }
  return hModule;
}

static void* GetProcAddress(HINSTANCE hModule, const char* func_name) {
  if (hModule == nullptr || func_name == NULL) {
    return NULL;
  }

  dlerror();
  void* func_ptr = dlsym(hModule, func_name);
  const char* err = dlerror();
  if (err != NULL) {
    fprintf(stderr, "获取函数%s失败: %s\n", func_name, err);
    return NULL;
  }
  return func_ptr;
}

static int FreeLibrary(HINSTANCE hModule) {
  if (hModule == nullptr) {
    return 0;
  }

  int ret = dlclose(hModule);
  if (ret != 0) {
    fprintf(stderr, "卸载库失败: %s\n", dlerror());
    return 0;
  }
  return 1;
}
#endif

#define _DEEPOON_SDK_DEFINE_FUNCTION_(r, f, p) \
  typedef r(*typedef_##f) p;                   \
  typedef_##f f;

#define TO_STR_IS_NULL(x) (_T(#x) _T(" is NULL\n"))
#define CK_PTR_NULL(x)                              \
  do {                                              \
    if ((x) == 0) {                                 \
      OutputDebugString(TO_STR_IS_NULL(x));         \
      /*MessageBox(0,TO_STR_IS_NULL(x),_T(""),0);*/ \
      return 0;                                     \
    }                                               \
  } while (0)

#define _DEEPOON_SDK_FIND_FUNCTION_(x, y)                \
  x##Ptr = (typedef_##x##Ptr)GetProcAddress(hDll, #x);   \
  if (x##Ptr == NULL) {                                  \
    x##Ptr = (typedef_##x##Ptr)GetProcAddress(hDll, #y); \
    if (x##Ptr == NULL) {                                \
      OutputDebugString(TO_STR_IS_NULL(x##Ptr));         \
    }                                                    \
  }

#define _DEEPOON_SDK_PERIPHERAL_FUNCTION_(x)                  \
  x##Ptr = (typedef_##x##Ptr)GetProcAddress(hDll, #x "Impl"); \
  if (x##Ptr == NULL) {                                       \
    OutputDebugString(TO_STR_IS_NULL(x##Ptr));                \
  }

#ifdef _WIN32
#ifdef _WIN64

#ifdef _DEBUG
#define DPN_PLATFORM_DLL _T("DpvrPlatform_x64d.dll")
#else
#define DPN_PLATFORM_DLL _T("DpvrPlatform_x64.dll")
#endif

#else

#ifdef _DEBUG
#define DPN_PLATFORM_DLL _T("DpvrPlatform_x86d.dll")
#else
#define DPN_PLATFORM_DLL _T("DpvrPlatform_x86.dll")
#endif
#endif
#else
#define DPN_PLATFORM_DLL "/opt/dpvr/assistant4/sdk/bin/libDpvrPlatform_x64.so"
#endif

HINSTANCE hDll = NULL;

extern "C" {
_DEEPOON_SDK_DEFINE_FUNCTION_(int, DpnnGetVersionPtr, ());
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnGetClientCountPtr,
                              (int* client_num,
                               int* render_client_num,
                               bool* steam_running,
                               DWORD* render_pid));
_DEEPOON_SDK_DEFINE_FUNCTION_(dpnnInstance,
                              DpnnInitPtr,
                              (int apiVersion,
                               dpnnUsageMode mode,
                               void* device,
                               dpnnDeviceType device_type,
                               void* user_data));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnGetDeviceInfoPtr,
                              (dpnnInstance instance,
                               dpnnDeviceInfo* device_info));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnSetTexturePtr,
                              (dpnnInstance instance,
                               void* texture,
                               dpnnEyeType eye_type,
                               dpnnTwType time_warping_type,
                               int width,
                               int height));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnRecordPosePtr,
                              (dpnnInstance instance, dpnnEyeType eye_type));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool, DpnnComposePtr, (dpnnInstance instance));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool, DpnnDeinitPtr, (dpnnInstance instance));
_DEEPOON_SDK_DEFINE_FUNCTION_(const dpnnQuarterion,
                              DpnnGetPosePtr,
                              (dpnnInstance instance));
_DEEPOON_SDK_DEFINE_FUNCTION_(const dpnnVector3,
                              DpnnGetPositionPtr,
                              (dpnnInstance instance));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnGetSensorDataPtr,
                              (dpnnInstance instance,
                               dpnnSensorData* const data));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnGetSensorRawDataPtr,
                              (dpnnInstance instance,
                               dpnnSensorRawData* const data));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnVerifySignaturePtr,
                              (void* signiture_data,
                               unsigned int signiture_len));
_DEEPOON_SDK_DEFINE_FUNCTION_(void*,
                              DpnnGetUserDataPtr,
                              (dpnnInstance instance));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnnSetUserDataPtr,
                              (dpnnInstance instance, void* userData));
// version 1.2 functions
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnPausePtr,
                              (dpnnInstance instance, int type));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnResumePtr,
                              (dpnnInstance instance, int type));
_DEEPOON_SDK_DEFINE_FUNCTION_(float,
                              DpnnGetFloatValuePtr,
                              (dpnnInstance instance,
                               const char* property_name,
                               float default_value));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnSetFloatValuePtr,
                              (dpnnInstance instance,
                               const char* property_name,
                               float value));
_DEEPOON_SDK_DEFINE_FUNCTION_(int,
                              DpnnGetLastErrorPtr,
                              (DPNN_INSTANCE instance));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnSetPtrValuePtr,
                              (DPNN_INSTANCE instance,
                               const char* property_name,
                               void* value));
_DEEPOON_SDK_DEFINE_FUNCTION_(void*,
                              DpnnGetPtrValuePtr,
                              (DPNN_INSTANCE instance,
                               const char* property_name,
                               void* default_value));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnSetIntValuePtr,
                              (DPNN_INSTANCE instance,
                               const char* property_name,
                               int value));
_DEEPOON_SDK_DEFINE_FUNCTION_(int,
                              DpnnGetIntValuePtr,
                              (DPNN_INSTANCE instance,
                               const char* property_name,
                               int default_value));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnUpdatePosePtr,
                              (DPNN_INSTANCE instance,
                               DPNN_QUARTERION* pose,
                               DPNN_VECTOR3* position));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnUpdatePose2Ptr,
                              (DPNN_INSTANCE instance, int32_t cacheIndex));
_DEEPOON_SDK_DEFINE_FUNCTION_(double, DpnnGetTimePtr, (DPNN_INSTANCE instance));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool, DpnnResetPosePtr, (DPNN_INSTANCE instance));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnnGetPredictedDisplayTimePtr,
                              (DPNN_INSTANCE instance,
                               double* predicated_time,
                               unsigned long long* frame_index));
_DEEPOON_SDK_DEFINE_FUNCTION_(DPNN_PREDICTION*,
                              DpnnPredictPtr,
                              (DPNN_INSTANCE instance));
_DEEPOON_SDK_DEFINE_FUNCTION_(const DPNN_QUARTERION,
                              DpnnGetPredictedPosePtr,
                              (DPNN_INSTANCE instance, double timeInSeconds));
_DEEPOON_SDK_DEFINE_FUNCTION_(const DPNN_VECTOR3,
                              DpnnGetPredictedPositionPtr,
                              (DPNN_INSTANCE instance, double timeInSeconds));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnSetTextureExPtr,
                              (DPNN_INSTANCE instance,
                               int index,
                               void* texture,
                               dpnnEyeType eye_type,
                               dpnnTwType time_warping_type,
                               DPNN_RECT viewport));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnRemoveLayerPtr,
                              (DPNN_INSTANCE instance, int index));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnSetLayerVisibilityPtr,
                              (DPNN_INSTANCE instance,
                               int index,
                               bool visible));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnStatisticsBeginPtr,
                              (DPNN_INSTANCE instance,
                               char* developer_id,
                               char* app_id));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnStatisticsEndPtr,
                              (DPNN_INSTANCE instance));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnnCreateSwapTextureSetPtr,
                              (DPNN_INSTANCE instance,
                               uint32_t unPid,
                               uint32_t unFormat,
                               uint32_t unWidth,
                               uint32_t unHeight,
                               uint64_t (*pSharedTextureHandles)[3]));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnnDestroySwapTextureSetPtr,
                              (DPNN_INSTANCE instance,
                               uint64_t sharedTextureHandle));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnnDestroyAllSwapTextureSetsPtr,
                              (DPNN_INSTANCE instance, uint32_t unPid));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnnGetNextSwapTextureSetIndexPtr,
                              (DPNN_INSTANCE instance,
                               uint64_t sharedTextureHandles[2],
                               uint32_t (*pIndices)[2]));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnnSubmitLayerPtr,
                              (DPNN_INSTANCE instance,
                               uint64_t sharedTextureHandles[2],
                               const DPNN_RECT (&bounds)[2],
                               const DPNN_QUARTERION* pose));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnnPresentPtr,
                              (DPNN_INSTANCE instance, uint64_t syncTexture));

// version 2.0 functions
_DEEPOON_SDK_DEFINE_FUNCTION_(void, DpnpPausePtr, (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(void, DpnpResumePtr, (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(int, DpnpQueryDeviceCountPtr, (DPNP_DEVICE_TYPE));
_DEEPOON_SDK_DEFINE_FUNCTION_(const char*,
                              DpnpGetDeviceIdPtr,
                              (DPNP_DEVICE_TYPE, int));
_DEEPOON_SDK_DEFINE_FUNCTION_(DPNP_DEVICE, DpnpOpenDevicePtr, (const char*));
_DEEPOON_SDK_DEFINE_FUNCTION_(void, DpnpCloseDevicePtr, (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(int,
                              DpnpGetDeviceTransformCountPtr,
                              (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(int, DpnpGetDevicePoseCountPtr, (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(int,
                              DpnpGetDevicePositionCountPtr,
                              (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(int, DpnpGetDeviceTimeCountPtr, (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(int,
                              DpnpGetDeviceQuaternionCountPtr,
                              (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(int, DpnpGetDeviceVectorCountPtr, (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(int, DpnpGetDeviceAxisCountPtr, (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(int, DpnpGetDeviceAnalogCountPtr, (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(int, DpnpGetDeviceButtonCountPtr, (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(int,
                              DpnpGetDeviceAttributeCountPtr,
                              (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(int,
                              DpnpGetDeviceFeedbackCountPtr,
                              (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(const char*, DpnpReadDeviceIdPtr, (DPNP_DEVICE));
_DEEPOON_SDK_DEFINE_FUNCTION_(const char*,
                              DpnpGetAssociatedDevicePtr,
                              (DPNP_DEVICE, DPNP_DEVICE_TYPE));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnpReadDeviceTransformPtr,
                              (DPNP_DEVICE, int, dpnpTransform*));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnpReadDevicePosePtr,
                              (DPNP_DEVICE, int, float[]));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnpReadDevicePositionPtr,
                              (DPNP_DEVICE, int, float[]));
_DEEPOON_SDK_DEFINE_FUNCTION_(double,
                              DpnpReadDeviceTimePtr,
                              (DPNP_DEVICE, int));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnpReadDeviceQuaternionPtr,
                              (DPNP_DEVICE, int, float[]));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnpReadDeviceVectorPtr,
                              (DPNP_DEVICE, int, float[]));
_DEEPOON_SDK_DEFINE_FUNCTION_(float, DpnpReadDeviceAxisPtr, (DPNP_DEVICE, int));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnpReadDeviceAnalogPtr,
                              (DPNP_DEVICE, int, float[]));
_DEEPOON_SDK_DEFINE_FUNCTION_(int, DpnpReadDeviceButtonPtr, (DPNP_DEVICE, int));
_DEEPOON_SDK_DEFINE_FUNCTION_(int,
                              DpnpReadDeviceAttributePtr,
                              (DPNP_DEVICE, int, void*, int));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnpSetDeviceAttributePtr,
                              (DPNP_DEVICE, int, void*, int));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnpResetDevicePosePtr,
                              (DPNP_DEVICE, int, float[]));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnpResetDevicePositionPtr,
                              (DPNP_DEVICE, int, float[]));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnpRegisterReferencePoseFunctionPtr,
                              (DPNP_DEVICE, int, DpnpReadPoseFunc, void*));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnpRegisterRererencePositionFunctionPtr,
                              (DPNP_DEVICE, int, DpnpReadPositionFunc, void*));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnpRegisterEventNotificationFunctionPtr,
                              (DPNP_DEVICE, DpnpHandleEventFunc, int, void*));
_DEEPOON_SDK_DEFINE_FUNCTION_(void,
                              DpnpWriteDeviceFeedbackPtr,
                              (DPNP_DEVICE, int, float));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnSetStringValuePtr,
                              (DPNN_INSTANCE instance,
                               const char* property_name,
                               char* value,
                               int max_len));
_DEEPOON_SDK_DEFINE_FUNCTION_(bool,
                              DpnnGetStringValuePtr,
                              (DPNN_INSTANCE instance,
                               const char* property_name,
                               char* default_value,
                               char* result,
                               int max_len,
                               int* len));
}

#ifdef _WIN32
static HMODULE GetCurrentModule() {
  // Note: XP+ solution!
  HMODULE hModule = NULL;
  GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                         GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     (LPCSTR)GetCurrentModule, &hModule);

  return hModule;
}

static BOOL GetSDKPathFromCurrentModulePath(TCHAR* path, DWORD size) {
  HMODULE phModule = GetCurrentModule();
  if (phModule != NULL) {
    DWORD dwRet = GetModuleFileName(phModule, path, size);
    if (dwRet != 0) {
      for (int i = 0; i < 3; i++) {
        TCHAR* pChr = _tcsrchr(path, _T('\\'));
        if (pChr == NULL) {
          return FALSE;
        }
        *pChr = _T('\0');
      }
      _tcscat_s(path, size, _T("\\SDK"));
      return TRUE;
    }
  }

  return FALSE;
}
#endif

static BOOL LoadSdkDll() {
#ifdef _WIN32
  TCHAR* dllPathName = new TCHAR[512];
  // load dll
  _tcscpy_s(dllPathName, 512, DPN_PLATFORM_DLL);

  if (hDll == NULL)  // using registry path
  {
    LONG lResult;
    DWORD len = 512 * sizeof(TCHAR);
    lResult = RegGetValue(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\DPVR4"),
                          _T("DPVR4_PLATFORM_DIR"),
                          RRF_RT_ANY | 0x00010000 /*RRF_SUBKEY_WOW6464KEY*/,
                          NULL, dllPathName, &len);
    if (lResult == ERROR_SUCCESS) {
      _tcscat_s(dllPathName, 512, _T("\\bin\\"));
      _tcscat_s(dllPathName, 512, DPN_PLATFORM_DLL);
      hDll = LoadLibrary(dllPathName);
    }
  }
  if (hDll == NULL)  // using DPVR4_PLATFORM_DIR
  {
    DWORD ret_size =
        GetEnvironmentVariable(L"DPVR4_PLATFORM_DIR", dllPathName, 512);
    if (ret_size != 0) {
      _tcscat_s(dllPathName, 512, _T("\\bin\\"));
      _tcscat_s(dllPathName, 512, DPN_PLATFORM_DLL);
      hDll = LoadLibrary(dllPathName);
    }
  }

  // using current dll or exe path: save SDK in steam driver case
  if (hDll == NULL) {
    if (GetSDKPathFromCurrentModulePath(dllPathName, 512)) {
      _tcscat_s(dllPathName, 512, _T("\\bin\\"));
      _tcscat_s(dllPathName, 512, DPN_PLATFORM_DLL);
      hDll = LoadLibrary(dllPathName);
    }
  }

  if (hDll == NULL) {
    hDll = LoadLibrary(DPN_PLATFORM_DLL);
  }

  if (hDll == NULL) {
    memset(dllPathName, 0, 512 * sizeof(TCHAR));
    _tcscat_s(dllPathName, 512, _T("Failed to load required Dll: "));
    _tcscat_s(dllPathName, 512, DPN_PLATFORM_DLL);
    _tcscat_s(dllPathName, 512, _T(", please install DPVR Assistant4!"));
    MessageBox(NULL, dllPathName, _T("Error"), MB_OK | MB_ICONSTOP);
    return FALSE;
  }
  delete[] dllPathName;
#else
  hDll = LoadLibrary(DPN_PLATFORM_DLL);
  if (hDll == nullptr) {
    return FALSE;
  }
#endif

  // load the functions
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetVersion, dpnnGetVersion);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetClientCount, dpnnGetClientCount);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnInit, dpnnInit);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetDeviceInfo, dpnnGetDeviceInfo);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnSetTexture, dpnnSetTexture);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnRecordPose, dpnnRecordPose);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnCompose, dpnnCompose);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnDeinit, dpnnDeinit);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetPose, dpnnGetPose);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetPosition, dpnnGetPosition);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetSensorData, dpnnGetSensorData);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetSensorRawData, dpnnGetSensorRawData);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnVerifySignature, dpnnVerifySignature);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetUserData, dpnnGetUserData);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnSetUserData, dpnnSetUserData);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnSetStringValue, dpnnSetStringValue);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetStringValue, dpnnGetStringValue);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnPause, dpnnPause);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnResume, dpnnResume);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetFloatValue, dpnnGetFloatValue);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnSetFloatValue, dpnnSetFloatValue);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetLastError, dpnnGetLastError);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnSetPtrValue, dpnnSetPtrValue);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetPtrValue, dpnnGetPtrValue);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnSetIntValue, dpnnSetIntValue);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetIntValue, dpnnGetIntValue);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnUpdatePose, dpnnUpdatePose);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnUpdatePose2, dpnnUpdatePose2);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetTime, dpnnGetTime);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnResetPose, dpnnResetPose);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetPredictedDisplayTime,
                              dpnnGetPredictedDisplayTime);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnPredict, dpnnPredict);

  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetPredictedPose, dpnnGetPredictedPose);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetPredictedPosition,
                              dpnnGetPredictedPosition);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnSetTextureEx, dpnnSetTextureEx);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnRemoveLayer, dpnnRemoveLayer);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnSetLayerVisibility, dpnnSetLayerVisibility);

  _DEEPOON_SDK_FIND_FUNCTION_(DpnnStatisticsBegin, dpnnStatisticsBegin);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnStatisticsEnd, dpnnStatisticsEnd);

  _DEEPOON_SDK_FIND_FUNCTION_(DpnnCreateSwapTextureSet,
                              dpnnCreateSwapTextureSet);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnDestroySwapTextureSet,
                              dpnnDestroySwapTextureSet);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnDestroyAllSwapTextureSets,
                              dpnnDestroyAllSwapTextureSets);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnGetNextSwapTextureSetIndex,
                              dpnnGetNextSwapTextureSetIndex);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnSubmitLayer, dpnnSubmitLayer);
  _DEEPOON_SDK_FIND_FUNCTION_(DpnnPresent, dpnnSubmitLayer);

  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpPause);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpResume);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpQueryDeviceCount);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpGetDeviceId);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpOpenDevice);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpCloseDevice);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpGetDeviceTransformCount);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpGetDevicePoseCount);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpGetDevicePositionCount);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpGetDeviceTimeCount);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpGetDeviceQuaternionCount);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpGetDeviceVectorCount);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpGetDeviceAxisCount);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpGetDeviceAnalogCount);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpGetDeviceButtonCount);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpGetDeviceAttributeCount);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpGetDeviceFeedbackCount);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpReadDeviceId);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpGetAssociatedDevice);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpReadDeviceTransform);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpReadDevicePose);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpReadDevicePosition);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpReadDeviceTime);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpReadDeviceQuaternion);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpReadDeviceVector);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpReadDeviceAxis);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpReadDeviceAnalog);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpReadDeviceButton);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpReadDeviceAttribute);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpSetDeviceAttribute);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpResetDevicePose);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpResetDevicePosition);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpRegisterReferencePoseFunction);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpRegisterRererencePositionFunction);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpRegisterEventNotificationFunction);
  _DEEPOON_SDK_PERIPHERAL_FUNCTION_(DpnpWriteDeviceFeedback);
  return TRUE;
}

DPN_API int DpnnGetVersion() {
  if (DpnnGetVersionPtr == nullptr) {
    if (!LoadSdkDll())
      return 0;
  }
  if (DpnnGetVersionPtr == nullptr) {
    OutputDebugStringA("DpnnGetVersionPtr is null\n");
    return 0;
  } else
    return DpnnGetVersionPtr();
}

DPN_API bool DpnnGetClientCount(int* client_num,
                                int* render_client_num,
                                bool* steam_running,
                                DWORD* render_pid) {
  if (DpnnGetClientCountPtr == nullptr) {
    if (!LoadSdkDll())
      return false;
  }
  if (DpnnGetClientCountPtr == nullptr) {
    OutputDebugStringA("DpnnGetClientCountPtr is null\n");
    return false;
  } else
    return DpnnGetClientCountPtr(client_num, render_client_num, steam_running,
                                 render_pid);
}

DPN_API dpnnInstance DpnnInit(int api_version,
                              dpnnUsageMode mode,
                              void* device,
                              dpnnDeviceType device_type,
                              void* user_data) {
  if (DpnnInitPtr == nullptr) {
    if (!LoadSdkDll())
      return nullptr;
  }
  if (DpnnInitPtr == nullptr) {
    OutputDebugStringA("DpnnInitPtr is null\n");
    return nullptr;
  } else
    return DpnnInitPtr(api_version, mode, device, device_type, user_data);
}

DPN_API bool DpnnGetDeviceInfo(dpnnInstance instance,
                               dpnnDeviceInfo* device_info) {
  if (DpnnGetDeviceInfoPtr == nullptr) {
    OutputDebugStringA("DpnnGetDeviceInfoPtr is null\n");
    return false;
  }
  return DpnnGetDeviceInfoPtr(instance, device_info);
}
DPN_API bool DpnnSetTexture(dpnnInstance instance,
                            void* texture,
                            dpnnEyeType eye_type,
                            dpnnTwType time_warping_type,
                            int width,
                            int height) {
  if (DpnnSetTexturePtr == nullptr) {
    OutputDebugStringA("DpnnSetTexturePtr is null\n");
    return false;
  }
  return DpnnSetTexturePtr(instance, texture, eye_type, time_warping_type,
                           width, height);
}
DPN_API bool DpnnRecordPose(dpnnInstance instance, dpnnEyeType eye_type) {
  if (DpnnRecordPosePtr == nullptr) {
    OutputDebugStringA("DpnnRecordPosePtr is null\n");
    return false;
  }
  return DpnnRecordPosePtr(instance, eye_type);
}
DPN_API bool DpnnCompose(dpnnInstance instance) {
  if (DpnnComposePtr == nullptr) {
    OutputDebugStringA("DpnnComposePtr is null\n");
    return false;
  }
  return DpnnComposePtr(instance);
}
DPN_API bool DpnnDeinit(dpnnInstance instance) {
  if (DpnnDeinitPtr == nullptr) {
    OutputDebugStringA("DpnnDeinitPtr is null\n");
    return false;
  }
  bool Deinit = DpnnDeinitPtr(instance);

  DpnnGetVersionPtr = NULL;
  DpnnInitPtr = NULL;
  DpnnGetDeviceInfoPtr = NULL;
  DpnnSetTexturePtr = NULL;
  DpnnRecordPosePtr = NULL;
  DpnnComposePtr = NULL;
  DpnnDeinitPtr = NULL;
  DpnnGetPosePtr = NULL;
  DpnnGetPositionPtr = NULL;
  DpnnGetSensorDataPtr = NULL;
  DpnnGetSensorRawDataPtr = NULL;
  DpnnVerifySignaturePtr = NULL;
  DpnnGetUserDataPtr = NULL;
  DpnnSetUserDataPtr = NULL;

  DpnnPausePtr = NULL;
  DpnnResumePtr = NULL;
  DpnnGetFloatValuePtr = NULL;
  DpnnSetFloatValuePtr = NULL;
  DpnnGetLastErrorPtr = NULL;
  DpnnSetPtrValuePtr = NULL;
  DpnnGetPtrValuePtr = NULL;
  DpnnSetIntValuePtr = NULL;
  DpnnGetIntValuePtr = NULL;
  DpnnUpdatePosePtr = NULL;
  DpnnUpdatePose2Ptr = NULL;
  DpnnGetTimePtr = NULL;
  DpnnResetPosePtr = NULL;
  DpnnGetPredictedDisplayTimePtr = NULL;
  DpnnPredictPtr = NULL;
  DpnnGetPredictedPosePtr = NULL;
  DpnnGetPredictedPositionPtr = NULL;
  DpnnSetTextureExPtr = NULL;
  DpnnRemoveLayerPtr = NULL;
  DpnnSetLayerVisibilityPtr = NULL;
  DpnnStatisticsBeginPtr = NULL;
  DpnnStatisticsEndPtr = NULL;

  DpnpPausePtr = NULL;
  DpnpResumePtr = NULL;
  DpnpQueryDeviceCountPtr = NULL;
  DpnpGetDeviceIdPtr = NULL;
  DpnpOpenDevicePtr = NULL;
  DpnpCloseDevicePtr = NULL;
  DpnpGetDeviceTransformCountPtr = NULL;
  DpnpGetDevicePoseCountPtr = NULL;
  DpnpGetDevicePositionCountPtr = NULL;
  DpnpGetDeviceTimeCountPtr = NULL;
  DpnpGetDeviceQuaternionCountPtr = NULL;
  DpnpGetDeviceVectorCountPtr = NULL;
  DpnpGetDeviceAxisCountPtr = NULL;
  DpnpGetDeviceAnalogCountPtr = NULL;
  DpnpGetDeviceButtonCountPtr = NULL;
  DpnpGetDeviceAttributeCountPtr = NULL;
  DpnpGetDeviceFeedbackCountPtr = NULL;
  DpnpReadDeviceIdPtr = NULL;
  DpnpGetAssociatedDevicePtr = NULL;
  DpnpReadDeviceTransformPtr = NULL;
  DpnpReadDevicePosePtr = NULL;
  DpnpReadDevicePositionPtr = NULL;
  DpnpReadDeviceTimePtr = NULL;
  DpnpReadDeviceQuaternionPtr = NULL;
  DpnpReadDeviceVectorPtr = NULL;
  DpnpReadDeviceAxisPtr = NULL;
  DpnpReadDeviceAnalogPtr = NULL;
  DpnpReadDeviceButtonPtr = NULL;
  DpnpReadDeviceAttributePtr = NULL;
  DpnpSetDeviceAttributePtr = NULL;
  DpnpResetDevicePosePtr = NULL;
  DpnpResetDevicePositionPtr = NULL;
  DpnpRegisterReferencePoseFunctionPtr = NULL;
  DpnpRegisterRererencePositionFunctionPtr = NULL;
  DpnpRegisterEventNotificationFunctionPtr = NULL;
  DpnpWriteDeviceFeedbackPtr = NULL;

  FreeLibrary(hDll);
  hDll = NULL;
  return Deinit;
}

DPN_API const dpnnQuarterion DpnnGetPose(dpnnInstance instance) {
  if (DpnnGetPosePtr == nullptr) {
    OutputDebugStringA("DpnnGetPosePtr is null\n");
    dpnnQuarterion ret = {1, 0, 0, 0};
    return ret;
  }
  return DpnnGetPosePtr(instance);
}

DPN_API const dpnnVector3 DpnnGetPosition(dpnnInstance instance) {
  if (DpnnGetPositionPtr == nullptr) {
    OutputDebugStringA("DpnnGetPositionPtr is null\n");
    dpnnVector3 ret;
    memset(&ret, 0, sizeof(ret));
    return ret;
  }
  return DpnnGetPositionPtr(instance);
}
DPN_API bool DpnnGetSensorData(dpnnInstance instance,
                               dpnnSensorData* const data) {
  if (DpnnGetSensorDataPtr == nullptr) {
    OutputDebugStringA("DpnnGetSensorDataPtr is null\n");
    return false;
  }
  return DpnnGetSensorDataPtr(instance, data);
}
DPN_API bool DpnnGetSensorRawData(dpnnInstance instance,
                                  dpnnSensorRawData* const data) {
  if (DpnnGetSensorRawDataPtr == nullptr) {
    OutputDebugStringA("DpnnGetSensorRawDataPtr is null\n");
    return false;
  }
  return DpnnGetSensorRawDataPtr(instance, data);
}
DPN_API bool DpnnVerifySignature(void* signiture_data,
                                 unsigned int signiture_len) {
  if (DpnnVerifySignaturePtr == nullptr) {
    OutputDebugStringA("DpnnVerifySignaturePtr is null\n");
    return false;
  }
  return DpnnVerifySignaturePtr(signiture_data, signiture_len);
}
DPN_API void* DpnnGetUserData(dpnnInstance instance) {
  if (DpnnGetUserDataPtr == nullptr) {
    OutputDebugStringA("DpnnGetUserDataPtr is null\n");
    return nullptr;
  }
  return DpnnGetUserDataPtr(instance);
}
DPN_API void DpnnSetUserData(dpnnInstance instance, void* userData) {
  if (DpnnSetUserDataPtr == nullptr) {
    OutputDebugStringA("DpnnSetUserDataPtr is null\n");
    return;
  }
  DpnnSetUserDataPtr(instance, userData);
}

// version 1.2 functions
DPN_API bool DpnnPause(dpnnInstance instance, int type) {
  if (DpnnPausePtr == nullptr) {
    OutputDebugStringA("DpnnPausePtr is null\n");
    return false;
  }
  return DpnnPausePtr(instance, type);
}
DPN_API bool DpnnResume(dpnnInstance instance, int type) {
  if (DpnnResumePtr == nullptr) {
    OutputDebugStringA("DpnnResumePtr is null\n");
    return false;
  }
  return DpnnResumePtr(instance, type);
}
DPN_API float DpnnGetFloatValue(dpnnInstance instance,
                                const char* property_name,
                                float default_value) {
  if (DpnnGetFloatValuePtr == nullptr) {
    OutputDebugStringA("DpnnGetFloatValuePtr is null\n");
    return default_value;
  }
  return DpnnGetFloatValuePtr(instance, property_name, default_value);
}
DPN_API bool DpnnSetFloatValue(dpnnInstance instance,
                               const char* property_name,
                               float value) {
  if (DpnnSetFloatValuePtr == nullptr) {
    OutputDebugStringA("DpnnSetFloatValuePtr is null\n");
    return false;
  }
  return DpnnSetFloatValuePtr(instance, property_name, value);
}

DPN_API int DpnnGetLastError(dpnnInstance instance) {
  if (DpnnGetLastErrorPtr == nullptr) {
    OutputDebugStringA("DpnnGetLastErrorPtr is null\n");
    return 0;
  }
  return DpnnGetLastErrorPtr(instance);
}

DPN_API bool DpnnSetPtrValue(DPNN_INSTANCE instance,
                             const char* property_name,
                             void* value) {
  if (DpnnSetPtrValuePtr == nullptr) {
    OutputDebugStringA("DpnnSetPtrValuePtr is null\n");
    return false;
  }
  return DpnnSetPtrValuePtr(instance, property_name, value);
}
DPN_API void* DpnnGetPtrValue(DPNN_INSTANCE instance,
                              const char* property_name,
                              void* default_value) {
  if (DpnnGetPtrValuePtr == nullptr) {
    OutputDebugStringA("DpnnGetPtrValuePtr is null\n");
    return nullptr;
  }
  return DpnnGetPtrValuePtr(instance, property_name, default_value);
}
DPN_API bool DpnnSetIntValue(DPNN_INSTANCE instance,
                             const char* property_name,
                             int value) {
  if (DpnnSetIntValuePtr == nullptr) {
    OutputDebugStringA("DpnnSetIntValuePtr is null\n");
    return false;
  }
  return DpnnSetIntValuePtr(instance, property_name, value);
}
DPN_API int DpnnGetIntValue(DPNN_INSTANCE instance,
                            const char* property_name,
                            int default_value) {
  if (DpnnGetIntValuePtr == nullptr) {
    OutputDebugStringA("DpnnGetIntValuePtr is null\n");
    return default_value;
  }
  return DpnnGetIntValuePtr(instance, property_name, default_value);
}
DPN_API bool DpnnUpdatePose(DPNN_INSTANCE instance,
                            DPNN_QUARTERION* pose,
                            DPNN_VECTOR3* position) {
  if (DpnnUpdatePosePtr == nullptr) {
    OutputDebugStringA("DpnnUpdatePosePtr is null\n");
    return false;
  }
  return DpnnUpdatePosePtr(instance, pose, position);
}
DPN_API bool DpnnUpdatePose2(DPNN_INSTANCE instance, int32_t cacheIndex) {
  if (DpnnUpdatePosePtr == nullptr) {
    OutputDebugStringA("DpnnUpdatePose2Ptr is null\n");
    return false;
  }
  return DpnnUpdatePose2Ptr(instance, cacheIndex);
}

DPN_API double DpnnGetTime(DPNN_INSTANCE instance) {
  if (DpnnGetTimePtr == nullptr) {
    OutputDebugStringA("DpnnGetTimePtr is null\n");
    return 0.0;
  }
  return DpnnGetTimePtr(instance);
}

DPN_API bool DpnnResetPose(DPNN_INSTANCE instance) {
  if (DpnnGetTimePtr == nullptr) {
    OutputDebugStringA("DpnnResetPosePtr is null\n");
    return 0.0;
  }
  return DpnnResetPosePtr(instance);
}

DPN_API DPNN_PREDICTION* DpnnPredict(DPNN_INSTANCE instance) {
  return DpnnPredictPtr(instance);
}

DPN_API void DpnnGetPredictedDisplayTime(DPNN_INSTANCE instance,
                                         double* predicated_time,
                                         unsigned long long* frame_index) {
  if (DpnnGetPredictedDisplayTimePtr == nullptr) {
    OutputDebugStringA("DpnnGetPredictedDisplayTimePtr is null\n");
    *predicated_time = 0.0;
    *frame_index = 0;
    return;
  }

  return DpnnGetPredictedDisplayTimePtr(instance, predicated_time, frame_index);
}

DPN_API const DPNN_QUARTERION DpnnGetPredictedPose(DPNN_INSTANCE instance,
                                                   double timeInSeconds) {
  if (DpnnGetPredictedPosePtr == nullptr) {
    OutputDebugStringA("DpnnGetPredictedDisplayTimePtr is null\n");
    DPNN_QUARTERION aaa = {1, 0, 0, 0};
    return aaa;
  }
  return DpnnGetPredictedPosePtr(instance, timeInSeconds);
}

DPN_API const DPNN_VECTOR3 DpnnGetPredictedPosition(DPNN_INSTANCE instance,
                                                    double timeInSeconds) {
  if (DpnnGetPredictedPositionPtr == nullptr) {
    OutputDebugStringA("DpnnGetPredictedDisplayTimePtr is null\n");
    DPNN_VECTOR3 aaa = {0, 0, 0};
    return aaa;
  }
  return DpnnGetPredictedPositionPtr(instance, timeInSeconds);
}

DPN_API bool DpnnSetTextureEx(DPNN_INSTANCE instance,
                              int index,
                              void* texture,
                              dpnnEyeType eye_type,
                              dpnnTwType time_warping_type,
                              DPNN_RECT viewport) {
  if (DpnnSetTextureExPtr == nullptr) {
    OutputDebugStringA("DpnnSetTextureExPtr is null\n");
    return false;
  }
  return DpnnSetTextureExPtr(instance, index, texture, eye_type,
                             time_warping_type, viewport);
}

DPN_API bool DpnnRemoveLayer(DPNN_INSTANCE instance, int index) {
  if (DpnnRemoveLayerPtr == nullptr) {
    OutputDebugStringA("DpnnRemoveLayerPtr is null\n");
    return false;
  }
  return DpnnRemoveLayerPtr(instance, index);
}

DPN_API bool DpnnSetLayerVisibility(DPNN_INSTANCE instance,
                                    int index,
                                    bool visible) {
  if (DpnnSetLayerVisibilityPtr == nullptr) {
    OutputDebugStringA("DpnnSetLayerVisibilityPtr is null\n");
    return false;
  }
  return DpnnSetLayerVisibilityPtr(instance, index, visible);
}

DPN_API bool DpnnStatisticsBegin(DPNN_INSTANCE instance,
                                 char* developer_id,
                                 char* app_id) {
  if (DpnnStatisticsBeginPtr == nullptr) {
    OutputDebugStringA("DpnnStatisticsBeginPtr is null\n");
    return false;
  }
  return DpnnStatisticsBeginPtr(instance, developer_id, app_id);
}

DPN_API bool DpnnStatisticsEnd(DPNN_INSTANCE instance) {
  if (DpnnStatisticsEndPtr == nullptr) {
    OutputDebugStringA("DpnnStatisticsEndPtr is null\n");
    return false;
  }
  return DpnnStatisticsEndPtr(instance);
}

DPN_API void DpnnCreateSwapTextureSet(DPNN_INSTANCE instance,
                                      uint32_t unPid,
                                      uint32_t unFormat,
                                      uint32_t unWidth,
                                      uint32_t unHeight,
                                      uint64_t (*pSharedTextureHandles)[3]) {
  if (DpnnCreateSwapTextureSetPtr == nullptr) {
    OutputDebugStringA("DpnnCreateSwapTextureSetPtr is null\n");
    return;
  }
  return DpnnCreateSwapTextureSetPtr(instance, unPid, unFormat, unWidth,
                                     unHeight, pSharedTextureHandles);
}
DPN_API void DpnnDestroySwapTextureSet(DPNN_INSTANCE instance,
                                       uint64_t sharedTextureHandle) {
  if (DpnnDestroySwapTextureSetPtr == nullptr) {
    OutputDebugStringA("DpnnDestroySwapTextureSetPtr is null\n");
    return;
  }
  return DpnnDestroySwapTextureSetPtr(instance, sharedTextureHandle);
}

DPN_API void DpnnDestroyAllSwapTextureSets(DPNN_INSTANCE instance,
                                           uint32_t unPid) {
  if (DpnnDestroyAllSwapTextureSetsPtr == nullptr) {
    OutputDebugStringA("DpnnDestroyAllSwapTextureSetsPtr is null\n");
    return;
  }
  return DpnnDestroyAllSwapTextureSetsPtr(instance, unPid);
}

DPN_API void DpnnGetNextSwapTextureSetIndex(DPNN_INSTANCE instance,
                                            uint64_t sharedTextureHandles[2],
                                            uint32_t (*pIndices)[2]) {
  if (DpnnGetNextSwapTextureSetIndexPtr == nullptr) {
    OutputDebugStringA("DpnnGetNextSwapTextureSetIndexPtr is null\n");
    return;
  }
  return DpnnGetNextSwapTextureSetIndexPtr(instance, sharedTextureHandles,
                                           pIndices);
}

DPN_API void DpnnSubmitLayer(DPNN_INSTANCE instance,
                             uint64_t sharedTextureHandles[2],
                             const DPNN_RECT (&bounds)[2],
                             const DPNN_QUARTERION* pose) {
  if (DpnnSubmitLayerPtr == nullptr) {
    OutputDebugStringA("DpnnSubmitLayerPtr is null\n");
    return;
  }
  return DpnnSubmitLayerPtr(instance, sharedTextureHandles, bounds, pose);
}

DPN_API void DpnnPresent(DPNN_INSTANCE instance, uint64_t syncTexture) {
  if (DpnnPresentPtr == nullptr) {
    OutputDebugStringA("DpnnPresentPtr is null\n");
    return;
  }
  return DpnnPresentPtr(instance, syncTexture);
}

DPN_API int DpnpQueryDeviceCount(DPNP_DEVICE_TYPE type) {
  if (DpnpQueryDeviceCountPtr == nullptr) {
    OutputDebugStringA("DpnpQueryDeviceCountPtr is null\n");
    return 0;
  }
  return DpnpQueryDeviceCountPtr(type);
}

DPN_API const char* DpnpGetDeviceId(DPNP_DEVICE_TYPE type, int index) {
  if (DpnpGetDeviceIdPtr == nullptr) {
    OutputDebugStringA("DpnpGetDeviceIdPtr is null\n");
    return nullptr;
  }
  return DpnpGetDeviceIdPtr(type, index);
}

DPN_API DPNP_DEVICE DpnpOpenDevice(const char* device_id) {
  if (DpnpOpenDevicePtr == nullptr) {
    OutputDebugStringA("DpnpOpenDevicePtr is null\n");
    return nullptr;
  }
  return DpnpOpenDevicePtr(device_id);
}

DPN_API void DpnpPause(DPNP_DEVICE device) {
  if (DpnpPausePtr == nullptr) {
    OutputDebugStringA("DpnpPausePtr is null\n");
    return;
  }
  DpnpPausePtr(device);
}

DPN_API void DpnpResume(DPNP_DEVICE device) {
  if (DpnpResumePtr == nullptr) {
    OutputDebugStringA("DpnpResumePtr is null\n");
    return;
  }
  DpnpResumePtr(device);
}

DPN_API void DpnpCloseDevice(DPNP_DEVICE device) {
  if (DpnpCloseDevicePtr == nullptr) {
    OutputDebugStringA("DpnpCloseDevicePtr is null\n");
    return;
  }
  DpnpCloseDevicePtr(device);
}

DPN_API int DpnpGetDeviceTransformCount(DPNP_DEVICE device) {
  if (DpnpGetDeviceTransformCountPtr == nullptr) {
    OutputDebugStringA("DpnpGetDeviceTransformCountPtr is null\n");
    return 0;
  }
  return DpnpGetDeviceTransformCountPtr(device);
}

DPN_API int DpnpGetDevicePoseCount(DPNP_DEVICE device) {
  if (DpnpGetDevicePoseCountPtr == nullptr) {
    OutputDebugStringA("DpnpGetDevicePoseCountPtr is null\n");
    return 0;
  }
  return DpnpGetDevicePoseCountPtr(device);
}

DPN_API int DpnpGetDevicePositionCount(DPNP_DEVICE device) {
  if (DpnpGetDevicePositionCountPtr == nullptr) {
    OutputDebugStringA("DpnpGetDevicePositionCountPtr is null\n");
    return 0;
  }
  return DpnpGetDevicePositionCountPtr(device);
}

DPN_API int DpnpGetDeviceTimeCount(DPNP_DEVICE device) {
  if (DpnpGetDeviceTimeCountPtr == nullptr) {
    OutputDebugStringA("DpnpGetDeviceTimeCountPtr is null\n");
    return 0;
  }
  return DpnpGetDeviceTimeCountPtr(device);
}

DPN_API int DpnpGetDeviceQuaternionCount(DPNP_DEVICE device) {
  if (DpnpGetDeviceQuaternionCountPtr == nullptr) {
    OutputDebugStringA("DpnpGetDeviceQuaternionCountPtr is null\n");
    return 0;
  }
  return DpnpGetDeviceQuaternionCountPtr(device);
}

DPN_API int DpnpGetDeviceVectorCount(DPNP_DEVICE device) {
  if (DpnpGetDeviceVectorCountPtr == nullptr) {
    OutputDebugStringA("DpnpGetDeviceVectorCountPtr is null\n");
    return 0;
  }
  return DpnpGetDeviceVectorCountPtr(device);
}

DPN_API int DpnpGetDeviceAxisCount(DPNP_DEVICE device) {
  if (DpnpGetDeviceAxisCountPtr == nullptr) {
    OutputDebugStringA("DpnpGetDeviceAxisCountPtr is null\n");
    return 0;
  }
  return DpnpGetDeviceAxisCountPtr(device);
}

DPN_API int DpnpGetDeviceAnalogCount(DPNP_DEVICE device) {
  if (DpnpGetDeviceAnalogCountPtr == nullptr) {
    OutputDebugStringA("DpnpGetDeviceAnalogCountPtr is null\n");
    return 0;
  }
  return DpnpGetDeviceAnalogCountPtr(device);
}

DPN_API int DpnpGetDeviceButtonCount(DPNP_DEVICE device) {
  if (DpnpGetDeviceButtonCountPtr == nullptr) {
    OutputDebugStringA("DpnpGetDeviceButtonCountPtr is null\n");
    return 0;
  }
  return DpnpGetDeviceButtonCountPtr(device);
}

DPN_API int DpnpGetDeviceAttributeCount(DPNP_DEVICE device) {
  if (DpnpGetDeviceAttributeCountPtr == nullptr) {
    OutputDebugStringA("DpnpGetDeviceAttributeCountPtr is null\n");
    return 0;
  }
  return DpnpGetDeviceAttributeCountPtr(device);
}

DPN_API int DpnpGetDeviceFeedbackCount(DPNP_DEVICE device) {
  if (DpnpGetDeviceFeedbackCountPtr == nullptr) {
    OutputDebugStringA("DpnpGetDeviceFeedbackCountPtr is null\n");
    return 0;
  }
  return DpnpGetDeviceFeedbackCountPtr(device);
}

DPN_API const char* DpnpReadDeviceId(DPNP_DEVICE device) {
  if (DpnpReadDeviceIdPtr == nullptr) {
    OutputDebugStringA("DpnpReadDeviceIdPtr is null\n");
    return nullptr;
  }
  return DpnpReadDeviceIdPtr(device);
}

DPN_API const char* DpnpGetAssociatedDevice(DPNP_DEVICE device,
                                            DPNP_DEVICE_TYPE type) {
  if (DpnpGetAssociatedDevicePtr == nullptr) {
    OutputDebugStringA("DpnpGetAssociatedDevicePtr is null\n");
    return nullptr;
  }
  return DpnpGetAssociatedDevicePtr(device, type);
}

DPN_API bool DpnpReadDeviceTransform(DPNP_DEVICE device,
                                     int index,
                                     dpnpTransform* transform) {
  if (DpnpReadDeviceTransformPtr == nullptr) {
    OutputDebugStringA("DpnpReadDeviceTransformPtr is null\n");
    return false;
  }
  return DpnpReadDeviceTransformPtr(device, index, transform);
}

DPN_API bool DpnpReadDevicePose(DPNP_DEVICE device,
                                int index,
                                float pose[DPNP_VALUE_TYPE_SIZE_POSE]) {
  if (DpnpReadDevicePosePtr == nullptr) {
    OutputDebugStringA("DpnpReadDevicePosePtr is null\n");
    return false;
  }
  return DpnpReadDevicePosePtr(device, index, pose);
}

DPN_API bool DpnpReadDevicePosition(
    DPNP_DEVICE device,
    int index,
    float position[DPNP_VALUE_TYPE_SIZE_POSITION]) {
  if (DpnpReadDevicePositionPtr == nullptr) {
    OutputDebugStringA("DpnpReadDevicePositionPtr is null\n");
    return false;
  }
  return DpnpReadDevicePositionPtr(device, index, position);
}

DPN_API double DpnpReadDeviceTime(DPNP_DEVICE device, int index) {
  if (DpnpReadDeviceTimePtr == nullptr) {
    OutputDebugStringA("DpnpReadDeviceTimePtr is null\n");
    return 0.0;
  }
  return DpnpReadDeviceTimePtr(device, index);
}

DPN_API bool DpnpReadDeviceQuaternion(
    DPNP_DEVICE device,
    int index,
    float quaternion[DPNP_VALUE_TYPE_SIZE_QUATERNION]) {
  if (DpnpReadDeviceQuaternionPtr == nullptr) {
    OutputDebugStringA("DpnpReadDeviceQuaternionPtr is null\n");
    return false;
  }
  return DpnpReadDeviceQuaternionPtr(device, index, quaternion);
}

DPN_API bool DpnpReadDeviceVector(DPNP_DEVICE device,
                                  int index,
                                  float vector[DPNP_VALUE_TYPE_SIZE_VECTOR]) {
  if (DpnpReadDeviceVectorPtr == nullptr) {
    OutputDebugStringA("DpnpReadDeviceVectorPtr is null\n");
    return false;
  }
  return DpnpReadDeviceVectorPtr(device, index, vector);
}

DPN_API float DpnpReadDeviceAxis(DPNP_DEVICE device, int index) {
  if (DpnpReadDeviceAxisPtr == nullptr) {
    OutputDebugStringA("DpnpReadDeviceAxisPtr is null\n");
    return 0.0f;
  }
  return DpnpReadDeviceAxisPtr(device, index);
}

DPN_API bool DpnpReadDeviceAnalog(DPNP_DEVICE device,
                                  int index,
                                  float analog[DPNP_VALUE_TYPE_SIZE_ANALOG]) {
  if (DpnpReadDeviceAnalogPtr == nullptr) {
    OutputDebugStringA("DpnpReadDeviceAnalogPtr is null\n");
    return 0.0f;
  }
  return DpnpReadDeviceAnalogPtr(device, index, analog);
}

DPN_API int DpnpReadDeviceButton(DPNP_DEVICE device, int index) {
  if (DpnpReadDeviceButtonPtr == nullptr) {
    OutputDebugStringA("DpnpReadDeviceButtonPtr is null\n");
    return 0;
  }
  return DpnpReadDeviceButtonPtr(device, index);
}

DPN_API int DpnpReadDeviceAttribute(DPNP_DEVICE device,
                                    int index,
                                    void* buffer,
                                    int buffer_length) {
  if (DpnpReadDeviceAttributePtr == nullptr) {
    OutputDebugStringA("DpnpReadDeviceAttributePtr is null\n");
    return 0;
  }
  return DpnpReadDeviceAttributePtr(device, index, buffer, buffer_length);
}

DPN_API bool DpnpSetDeviceAttribute(DPNP_DEVICE device,
                                    int index,
                                    void* buffer,
                                    int buffer_length) {
  if (DpnpSetDeviceAttributePtr == nullptr) {
    OutputDebugStringA("DpnpSetDeviceAttributePtr is null\n");
    return false;
  }
  return DpnpSetDeviceAttributePtr(device, index, buffer, buffer_length);
}

DPN_API void DpnpResetDevicePose(DPNP_DEVICE device,
                                 int index,
                                 float pose[DPNP_VALUE_TYPE_SIZE_POSE]) {
  if (DpnpResetDevicePosePtr == nullptr) {
    OutputDebugStringA("DpnpResetDevicePosePtr is null\n");
    return;
  }
  DpnpResetDevicePosePtr(device, index, pose);
}

DPN_API void DpnpResetDevicePosition(
    DPNP_DEVICE device,
    int index,
    float position[DPNP_VALUE_TYPE_SIZE_POSITION]) {
  if (DpnpResetDevicePositionPtr == nullptr) {
    OutputDebugStringA("DpnpResetDevicePositionPtr is null\n");
    return;
  }
  DpnpResetDevicePositionPtr(device, index, position);
}

DPN_API void DpnpRegisterReferencePoseFunction(DPNP_DEVICE device,
                                               int index,
                                               DpnpReadPoseFunc callback,
                                               void* user_data) {
  if (DpnpRegisterReferencePoseFunctionPtr == nullptr) {
    OutputDebugStringA("DpnpRegisterReferencePoseFunctionPtr is null\n");
    return;
  }
  DpnpRegisterReferencePoseFunctionPtr(device, index, callback, user_data);
}

DPN_API void DpnpRegisterRererencePositionFunction(
    DPNP_DEVICE device,
    int index,
    DpnpReadPositionFunc callback,
    void* user_data) {
  if (DpnpRegisterRererencePositionFunctionPtr == nullptr) {
    OutputDebugStringA("DpnpRegisterRererencePositionFunctionPtr is null\n");
    return;
  }
  DpnpRegisterRererencePositionFunctionPtr(device, index, callback, user_data);
}

DPN_API void DpnpRegisterEventNotificationFunction(DPNP_DEVICE device,
                                                   DpnpHandleEventFunc callback,
                                                   int event_mask,
                                                   void* user_data) {
  if (DpnpRegisterEventNotificationFunctionPtr == nullptr) {
    OutputDebugStringA("DpnpRegisterEventNotificationFunctionPtr is null\n");
    return;
  }
  DpnpRegisterEventNotificationFunctionPtr(device, callback, event_mask,
                                           user_data);
}

DPN_API void DpnpWriteDeviceFeedback(DPNP_DEVICE device,
                                     int index,
                                     float value) {
  if (DpnpWriteDeviceFeedbackPtr == nullptr) {
    OutputDebugStringA("DpnpWriteDeviceFeedbackPtr is null\n");
    return;
  }
  DpnpWriteDeviceFeedbackPtr(device, index, value);
}
DPN_API bool DpnnSetStringValue(DPNN_INSTANCE instance,
                                const char* property_name,
                                char* value,
                                int max_len) {
  if (DpnnSetStringValuePtr == nullptr) {
    OutputDebugStringA("DpnnSetStringValue is null\n");
    return false;
  }
  return DpnnSetStringValuePtr(instance, property_name, value, max_len);
}
DPN_API bool DpnnGetStringValue(DPNN_INSTANCE instance,
                                const char* property_name,
                                char* default_value,
                                char* result,
                                int max_len,
                                int* len) {
  if (DpnnSetStringValuePtr == nullptr) {
    OutputDebugStringA("DpnnSetStringValue is null\n");
    return false;
  }
  return DpnnGetStringValuePtr(instance, property_name, default_value, result,
                               max_len, len);
}

static void SwitchControlMode(DPNP_DEVICE device) {
  DpnpWriteDeviceFeedback(
      device, DPNP_VALUE_TYPE_FEEDBACK_MOTOR - DPNP_VALUE_TYPE_FEEDBACK, 0.15f);
  Sleep(300);
  DpnpWriteDeviceFeedback(
      device, DPNP_VALUE_TYPE_FEEDBACK_MOTOR - DPNP_VALUE_TYPE_FEEDBACK, 0.15f);
}

static float DpnpControllerData(float x) {
  if (x < -1000) {
    return -1000;
  } else if (x < -500) {
    return 8.0f / 5 * (x + 1000) - 1000;
  } else if (x < -150) {
    return 4.0f / 7 * (x + 150);
  } else if (x < 150) {
    return 0.0f;
  } else if (x < 500) {
    return 4.0f / 7 * (x - 150);
  } else if (x < 1000) {
    return 8.0f / 5 * (x - 1000) + 1000;
  } else {
    return 1000;
  }
}

DPN_API void DpnpHandleControllerData(DPNP_DEVICE device,
                                      int hand,
                                      DpnnControllerRecordOriginal* src,
                                      DpnnControllerRecord* dest) {
  static float g_pre_touch_length[MAX_CONTROLLER_COUNT] = {0.0f};

  if (src->connect_state &
      (DPNP_DEVICE_STATUS_BASE_A_TRACK | DPNP_DEVICE_STATUS_BASE_B_TRACK)) {
    dest->controllerTransform.is_connected = 1;
    dest->controllerTransform.is_valid = 1;
  } else {
    dest->controllerTransform.is_connected = 0;
    dest->controllerTransform.is_valid = 0;
  }
  dest->controllerTransform.transform = src->transform;

  dest->controllerState.touchpad_analog_x = 0.0f;
  dest->controllerState.touchpad_analog_y = 0.0f;
  dest->controllerState.joystick_analog_x = 0.0f;
  dest->controllerState.joystick_analog_y = 0.0f;
  dest->controllerState.trigger_analog = 0.0f;
  dest->controllerState.grip_analog = 0.0f;
  dest->controllerState.battery_analog = 0.0f;
  dest->controllerState.button_clicked_flags = 0;
  dest->controllerState.button_touched_flags = 0;

  // System button (R) / ApplicationMenu (L)
  if (src->button_mask & DPNP_ELITE_BUTTONMASK_HOME_MENU) {
    dest->controllerState.button_clicked_flags |=
        DpnpButtonMask(DPNP_k_EButton_System);
    // dest->controllerState.button_touched_flags |=
    // DpnpButtonMask(DPNP_k_EButton_System);
  }

  // A (R) / X (L)
  if (src->button_mask & DPNP_ELITE_BUTTONMASK_XA) {
    dest->controllerState.button_clicked_flags |=
        hand ? DpnpButtonMask(DPNP_k_EButton_A)
             : DpnpButtonMask(DPNP_k_EButton_X);
    // dest->controllerState.button_touched_flags |= hand ?
    // DpnpButtonMask(DPNP_k_EButton_A) : DpnpButtonMask(DPNP_k_EButton_X);
  }

  // B (R) / Y (L)
  if (src->button_mask & DPNP_ELITE_BUTTONMASK_YB) {
    dest->controllerState.button_clicked_flags |=
        hand ? DpnpButtonMask(DPNP_k_EButton_B)
             : DpnpButtonMask(DPNP_k_EButton_Y);
    // dest->controllerState.button_touched_flags |= hand ?
    // DpnpButtonMask(DPNP_k_EButton_B) : DpnpButtonMask(DPNP_k_EButton_Y);
  }

  // Grip button
  dest->controllerState.grip_analog =
      src->analog[DPNP_ELITE_CONTROLLER_ANALOG_GRIP] / 1000.0f;
  if (src->button_mask & DPNP_ELITE_BUTTONMASK_GRIP) {
    dest->controllerState.button_clicked_flags |=
        DpnpButtonMask(DPNP_k_EButton_Grip);
    dest->controllerState.button_touched_flags |=
        DpnpButtonMask(DPNP_k_EButton_Grip);
  } else if (src->analog[DPNP_ELITE_CONTROLLER_ANALOG_GRIP]) {
    dest->controllerState.button_touched_flags |=
        DpnpButtonMask(DPNP_k_EButton_Grip);
  }

  // Trigger analog and button
  dest->controllerState.trigger_analog =
      src->analog[DPNP_ELITE_CONTROLLER_ANALOG_TRIGGER] / 1000.0f;
  if (src->button_mask & DPNP_ELITE_BUTTONMASK_TRIGER) {
    dest->controllerState.button_clicked_flags |=
        DpnpButtonMask(DPNP_k_EButton_Trigger);
    dest->controllerState.button_touched_flags |=
        DpnpButtonMask(DPNP_k_EButton_Trigger);
  } else if (src->analog[DPNP_ELITE_CONTROLLER_ANALOG_TRIGGER]) {
    dest->controllerState.button_touched_flags |=
        DpnpButtonMask(DPNP_k_EButton_Trigger);
  }
  dest->controllerState.battery_analog =
      src->analog[DPNP_ELITE_CONTROLLER_ANALOG_BATTERY];

  // Switch controller type

  // Joystick analog x, y
  // static double last_unpress_time[MAX_CONTROLLER_COUNT] = { 0 };
  float origin_length =
      (float)sqrt(src->analog[DPNP_ELITE_CONTROLLER_ANALOG_JOYSTICK_X] *
                      src->analog[DPNP_ELITE_CONTROLLER_ANALOG_JOYSTICK_X] +
                  src->analog[DPNP_ELITE_CONTROLLER_ANALOG_JOYSTICK_Y] *
                      src->analog[DPNP_ELITE_CONTROLLER_ANALOG_JOYSTICK_Y]);
  float touch_length = DpnpControllerData(origin_length);
  if (touch_length == 0)
    origin_length = 1.0f;
  float touchpad_analog_x =
      src->analog[DPNP_ELITE_CONTROLLER_ANALOG_JOYSTICK_X] * touch_length /
      (origin_length * 1000.0f);
  float touchpad_analog_y =
      src->analog[DPNP_ELITE_CONTROLLER_ANALOG_JOYSTICK_Y] * touch_length /
      (origin_length * 1000.0f);
  if (src->button_mask & DPNP_ELITE_BUTTONMASK_JOYSTICK) {
    if (true)  //(button_time - last_unpress_time[hand] > 0.1)
    {
      dest->controllerState.button_clicked_flags |=
          DpnpButtonMask(DPNP_k_EButton_Touchpad);
      dest->controllerState.button_touched_flags |=
          DpnpButtonMask(DPNP_k_EButton_Touchpad);
    } else {
      dest->controllerState.button_touched_flags |=
          DpnpButtonMask(DPNP_k_EButton_Touchpad);
    }
    dest->controllerState.button_clicked_flags |=
        DpnpButtonMask(DPNP_k_EButton_Joystick);
    dest->controllerState.button_touched_flags |=
        DpnpButtonMask(DPNP_k_EButton_Joystick);
  }
  if (touch_length > 0) {
    dest->controllerState.button_touched_flags |=
        DpnpButtonMask(DPNP_k_EButton_Joystick);
    dest->controllerState.joystick_analog_x = touchpad_analog_x;
    dest->controllerState.joystick_analog_y = touchpad_analog_y;
    dest->controllerState.touchpad_analog_x = touchpad_analog_x;
    dest->controllerState.touchpad_analog_y = touchpad_analog_y;
    if (touch_length > g_pre_touch_length[hand] * 0.95f) {
      dest->controllerState.button_touched_flags |=
          DpnpButtonMask(DPNP_k_EButton_Touchpad);
    } else  // fast recover
    {
      dest->controllerState.touchpad_analog_x = 0;
      dest->controllerState.touchpad_analog_y = 0;
      // last_unpress_time[hand] = button_time;
    }
  } else {
    dest->controllerState.touchpad_analog_x = 0;
    dest->controllerState.touchpad_analog_y = 0;
    // last_unpress_time[hand] = button_time;
  }
  dest->controllerState.packet_number++;
  g_pre_touch_length[hand] = touch_length;
}
