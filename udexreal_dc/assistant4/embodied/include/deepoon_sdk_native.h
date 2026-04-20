/************************************************************************************
Filename    :
Content     :
Created     :   Jan 4, 2016
Authors     :   deepoon.com
Copyright   :   Copyright 2016 Deepoon, Inc. All Rights reserved.
************************************************************************************/
#ifndef _DEEPOON_SDK_NATIVE_H_
#define _DEEPOON_SDK_NATIVE_H_

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define DPNN_RUNTIME_VERSION(major,minor,revision)	((major)<<24)|((minor)<<16)|(revision)
#define DPNN_LAYER_NUM_MAX 16

// enumerations
typedef enum tag_dpnnUsageMode {
	DPNN_UM_DEFAULT=0,
	DPNN_UM_SENSOR_ONLY=1,
	DPNN_UM_EDITOR_MODE = 2,
	DPNN_UM_DEEPOON_STEAM = 3,
	DPNN_UM_DEEPOON_VERIFY = 4,
    DPNN_UM_DEEPOON_SWAP_TEXTURE = 5,
	DPNN_UM_COUNT
} dpnnUsageMode, DPNN_USAGE_MODE;

typedef enum tag_dpnnDeviceType {
	DPNN_DEVICE_DX11=0,
	DPNN_DEVICE_DX10=1,
	DPNN_DEVICE_DX9=2,
	DPNN_DEVICE_GLES2=3,
	DPNN_DEVICE_GLES3=4,
	DPNN_DEVICE_COUNT
} dpnnDeviceType, DPNN_DEVICE_TYPE;

typedef enum tag_dpnnEyeType {
	DPNN_EYE_LEFT=0,
	DPNN_EYE_RIGHT=1,
	DPNN_EYE_CENTER=2,
	DPNN_EYE_LEFT_RIGHT=3,
	DPNN_EYE_COUNT
} dpnnEyeType, DPNN_EYE_TYPE;

typedef enum tag_dpnnTwType {
	DPNN_TW_NONE=0,
	DPNN_TW_DISTORTION=1,
	DPNN_TW_TW=2,
	DPNN_TW_TW_DISTORTION=3,
	DPNN_TW_SCREEN=4,
	DPNN_TW_TW_SCREEN=5,
	DPNN_TW_COUNT
} dpnnTwType, DPNN_TW_TYPE;

typedef enum tag_dpnnComposeType {
	DPNN_COMPOSE_BOTH=0,
	DPNN_COMPOSE_LEFT=1,
	DPNN_COMPOSE_RIGHT=2,
	DPNN_COMPOSE_COUNT
} DPNN_COMPOSE_TYPE;

// structure definitions
#pragma pack( 1 )
	typedef void * dpnnInstance;
	typedef dpnnInstance DPNN_INSTANCE;

	typedef struct
	{
		float s;
		float i;
		float j;
		float k;
	} dpnnQuarterion, DPNN_QUARTERION;

	typedef struct
	{
		float x;
		float y;
		float z;
	} dpnnVector3, DPNN_VECTOR3;

	typedef struct _DPNN_RECT
	{
		float x;
		float y;
		float w;
		float h;

		bool operator== (_DPNN_RECT rect)
		{
			return (x == rect.x) && (y == rect.y) && (w == rect.w) && (h == rect.h);
		}
	} dpnnRect, DPNN_RECT;

    typedef struct _DPNN_MATRIX
    {
        float m[4][4];
        float l, r, t, b;
    }DPNN_MATRIX;

	typedef struct
	{
		DPNN_VECTOR3 linear_acceleration; // Acceleration reading in m/s^2.
		DPNN_VECTOR3 angular_velocity;    // Rotation rate in rad/s.
		DPNN_VECTOR3 magnetometer;        // Magnetic field in Gauss.
	} dpnnSensorData, DPNN_SENSOR_DATA;

	typedef struct
	{
		DPNN_VECTOR3 linear_acceleration; // Acceleration reading in g.
		DPNN_VECTOR3 angular_velocity;    // Rotation rate in degree/s.
		DPNN_VECTOR3 magnetometer;        // Magnetic field in Gauss.
		float        temperature;         // Temperature of the sensor in degrees Celsius.
		double       time_in_seconds;     // Time when the reported IMU reading took place, in seconds.
	} dpnnSensorRawData, DPNN_SENSOR_RAW_DATA;

	typedef struct
	{
		float ipd;
		int resolution_x; // single eye
		int resolution_y; // single eye
		float fov_x;
		float fov_y;
	} dpnnDeviceInfo, DPNN_DEVICE_INFO;

	typedef struct Prediction
	{
		Prediction()
		{
			key = -1;
			time = 0;
			sampleTime = 0;
		}
		int key;
		DPNN_QUARTERION pose;
		DPNN_VECTOR3 position;
		double time;
		double sampleTime;
	}dpnnPrediction, DPNN_PREDICTION;

#pragma pack()

#ifdef WINVER // windows
#ifdef STATICLIB
#define DPN_API
#else
#ifdef DEEPOON_SHARED_LIBRARY // dll export
#define DPN_API extern "C" __declspec(dllexport)
#else // dll import for windows
#define DPN_API
#endif // DEEPOON_SHARED_LIBRARY
#endif // STATICLIB
#else // non window platform
#define DPN_API // no need to do declaration for android/linux
#endif

// version 1.2 functions for replacing old function names
DPN_API int		DpnnGetVersion();
DPN_API DPNN_INSTANCE DpnnInit(int api_version, DPNN_USAGE_MODE mode, void * device, DPNN_DEVICE_TYPE device_type, void *user_data);
DPN_API bool	DpnnGetDeviceInfo(DPNN_INSTANCE instance, DPNN_DEVICE_INFO *device_info);
DPN_API bool	DpnnSetTexture(DPNN_INSTANCE instance, void *texture, DPNN_EYE_TYPE eye_type, DPNN_TW_TYPE time_warping_type, int width, int height);
DPN_API bool	DpnnRecordPose(DPNN_INSTANCE instance, DPNN_EYE_TYPE eye_type);
DPN_API bool	DpnnCompose(DPNN_INSTANCE instance);
DPN_API bool	DpnnDeinit(DPNN_INSTANCE instance);
DPN_API const	DPNN_QUARTERION DpnnGetPose(DPNN_INSTANCE instance);
DPN_API const	DPNN_VECTOR3 DpnnGetPosition(DPNN_INSTANCE instance);
DPN_API bool	DpnnGetSensorData(DPNN_INSTANCE instance, DPNN_SENSOR_DATA * const data);
DPN_API bool	DpnnGetSensorRawData(DPNN_INSTANCE instance, DPNN_SENSOR_RAW_DATA * const data);
DPN_API bool	DpnnVerifySignature(void *signiture_data, unsigned int signiture_len);
DPN_API void *	DpnnGetUserData(DPNN_INSTANCE instance);
DPN_API void	DpnnSetUserData(DPNN_INSTANCE instance, void* userData);
DPN_API bool DpnnGetClientCount(int* client_num,
                                int* render_client_num,
                                bool* steam_running,
                                unsigned long* render_pid);

// version 1.2 functions
DPN_API bool	DpnnPause(DPNN_INSTANCE instance, int type);
DPN_API bool	DpnnResume(DPNN_INSTANCE instance, int type);
DPN_API float	DpnnGetFloatValue(DPNN_INSTANCE instance, const char *property_name, float default_value);
DPN_API bool	DpnnSetFloatValue(DPNN_INSTANCE instance, const char *property_name, float value);
DPN_API bool	DpnnSetPtrValue(DPNN_INSTANCE instance, const char *property_name, void * value);
DPN_API void *	DpnnGetPtrValue(DPNN_INSTANCE instance, const char *property_name, void *default_value);
DPN_API bool	DpnnSetIntValue(DPNN_INSTANCE instance, const char *property_name, int value);
DPN_API int		DpnnGetIntValue(DPNN_INSTANCE instance, const char *property_name, int default_value);
DPN_API bool	DpnnSetStringValue(DPNN_INSTANCE instance, const char *property_name, char * value,int max_len);
DPN_API bool	DpnnGetStringValue(DPNN_INSTANCE instance, const char *property_name, char *default_value, char *result, int max_len,int *len);
DPN_API bool    DpnnUpdatePose(DPNN_INSTANCE instance, DPNN_QUARTERION *pose, DPNN_VECTOR3 *position);
DPN_API bool    DpnnUpdatePose2(DPNN_INSTANCE instance, int32_t key);
DPN_API double  DpnnGetTime(DPNN_INSTANCE instance); // returns the time elapsed since the system on in seconds
DPN_API bool    DpnnSetVsyncTime(DPNN_INSTANCE instance, double vsyncTime); // set latest vsync time, the value should be obtained from DpnnGetTime in caller
DPN_API bool	DpnnResetPose(DPNN_INSTANCE instance);
DPN_API int		DpnnGetLastError(DPNN_INSTANCE instance);

 //Returns the predicated display time of current frame.
DPN_API void   DpnnGetPredictedDisplayTime(DPNN_INSTANCE instance, double * predicated_time, unsigned long long * frame_index);
DPN_API const DPNN_QUARTERION DpnnGetPredictedPose(DPNN_INSTANCE instance, double timeInSeconds);
DPN_API const DPNN_VECTOR3 DpnnGetPredictedPosition(DPNN_INSTANCE instance, double timeInSeconds);
DPN_API DPNN_PREDICTION*  DpnnPredict(DPNN_INSTANCE instance);

// multilayer used for GUI
DPN_API bool DpnnSetTextureEx(DPNN_INSTANCE instance, int index, void * texture, dpnnEyeType eye_type, dpnnTwType time_warping_type, DPNN_RECT viewport);
DPN_API bool DpnnRemoveLayer(DPNN_INSTANCE instance, int index);
DPN_API bool DpnnSetLayerVisibility(DPNN_INSTANCE instance, int index, bool visible);

DPN_API bool DpnnStatisticsBegin(DPNN_INSTANCE instance, char* developer_id, char* app_id);
DPN_API bool DpnnStatisticsEnd(DPNN_INSTANCE instance);

DPN_API void DpnnCreateSwapTextureSet(DPNN_INSTANCE instance, uint32_t unPid, uint32_t unFormat, uint32_t unWidth, uint32_t unHeight, uint64_t(*pSharedTextureHandles)[3]);
DPN_API void DpnnDestroySwapTextureSet(DPNN_INSTANCE instance, uint64_t sharedTextureHandle);
DPN_API void DpnnDestroyAllSwapTextureSets(DPNN_INSTANCE instance, uint32_t unPid);
DPN_API void DpnnGetNextSwapTextureSetIndex(DPNN_INSTANCE instance, uint64_t sharedTextureHandles[2], uint32_t(*pIndices)[2]);
DPN_API void DpnnSubmitLayer(DPNN_INSTANCE instance, uint64_t sharedTextureHandles[2], const DPNN_RECT(&bounds)[2], const DPNN_QUARTERION *pose, 
    const DPNN_MATRIX(&project)[2]);
DPN_API void DpnnPresent(DPNN_INSTANCE instance, uint64_t syncTexture);
DPN_API void DpnnPostPresent(DPNN_INSTANCE instance);
DPN_API void DpnnGetDiscreteAdapterLuid(DPNN_INSTANCE instance, uint64_t* luid);
DPN_API void DpnnTriggerHaptics(DPNN_INSTANCE instance, int device_mask, float factors[6]);
#ifdef __cplusplus
}
#endif
#endif /* _DEEPOON_SDK_NATIVE_H_ */
