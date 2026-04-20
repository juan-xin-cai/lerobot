#pragma once

enum class DpnpCommonEventID
{
	Unknown = 0,
	RoomSetupStart,
	RoomSetupFinish,
	ControllerOTAResult,

	// param : DpnpCommonEventParam_T2<int, string>
	// int index : 0 left, 1 right, 2 2.4G
	// std::string : fwversion
	ControllerFWVersion, //


	ControllerPairResult,

	// char
	SendControllerLedConfigResult,

	// std::string
	HMDFWVersion,

	// std::string
	HMDSerialNumber,

	// int index : 0 left, 1 right
	// std::string : sn
	ControllerSerialNumber,

	// char 0 left, 1 right
	// int level
	ControllerBattery,

	// usb state : 0 disconnect, 2 usb2, 3 usb3
	USBState,

	RequestToStartSteamVR,

	// int
	// > 0 : succeed
	// = 0 : doing
	// < 0 : failed result
	RequestToStartSteamVR_Result,

	// param
	// byte : state
	DeviceSleepStageChanged,

	OverlayResourceBundleNotFound, // DpnpCommonEventParam_T2<DpvrLanguage, std::string>

	ChangeLanguage, // DpvrLanguage

	CameraStateChanged, // DpnpCommonEventParam_T1<uint8>

	DeviceOpenSucceed,

	HID_Write, // DpnpCommonEventParam_T1<std::vector<uchar>>
	HID_WriteAndRead, // DpnpCommonEventParam_T1<std::vector<uchar>>
	RenderClientNumChanged,// DpnpCommonEventParam_T1<int>
	
	Overlayer_VisibleState, // DpnpCommonEventParam_T2<DpvrOverlayIndex, bool>
	Overlayer_CurrentStage, // DpnpCommonEventParam_T2<DpvrOverlayIndex, int>
	OverlaySystem_SeeThroughCalledByUser,// DpnpCommonEventParam_T1<bool>
	Slam_Lost, // DpnpCommonEventParam_T1<bool>

	PlayArea_FenceVisible,// DpnpCommonEventParam_T2<bool selfVisible, bool overlayerVisible>
	
	Recenter,// no param
	
	SET_ROOMSETUP_SIMPLE_MODE_HEIGHTS,// DpnpCommonEventParam_T4<DpnpCommonEvent, std::vector<int>, int , int> : eventID, heights, current_height_index, radius
	GET_ROOMSETUP_SIMPLE_MODE_HEIGHTS,// DpnpCommonEventParam_T4<DpnpCommonEvent, std::vector<int>, int , int> : eventID, heights, current_height_index, radius
	SET_ROOMSETUP_SIMPLE_MODE_HEIGHTS_RESULT,// DpnpCommonEventParam_T1<bool>

	RESET_SIMPLE_MOME_ROOMSETUP,// DpnpCommonEventParam_T1<int> : current_height_index

	WRITE_DEVICE_SLEEP_DURATION,// DpnpCommonEventParam_T3<byte, byte, byte> : hours, mins, seconds

	READ_DEVICE_SLEEP_DURATION,// DpnpCommonEventParam_T3<byte, byte, byte> : hours, mins, seconds

	UPDATE_ORIGIN, // DpnpCommonEventParam_T2<std::vector<float>, float> : new coord, delta yaw

	WRITE_DEVICE_PSENSOR_DISABLED,// DpnpCommonEventParam_T1<bool>

	READ_DEVICE_PSENSOR_DISABLED,// DpnpCommonEventParam_T1<bool>
};


static const char* dpnpCommonEventIDNames[] =
{
	"Unknown = 0",
	"RoomSetupStart",
	"RoomSetupFinish",
	"ControllerOTAResult",
	"ControllerFWVersion", //
	"ControllerPairResult",
	"SendControllerLedConfigResult",
	"HMDFWVersion",
	"HMDSerialNumber",

	"ControllerSerialNumber",
	"ControllerBattery",

	"USBState",

	"RequestToStartSteamVR",
	"RequestToStartSteamVR_Result",

	"DeviceSleepStageChanged",

	"OverlayResourceBundleNotFound",

	"ChangeLanguage",

	"CameraStateChanged",

	"DeviceOpenSucceed",

	"HID_Write",
	"HID_WriteAndRead",
	"RenderClientNumChanged",

	"Overlayer_VisibleState",
	"Overlayer_CurrentStage",
	"OverlaySystem_SeeThroughCalledByUser",
	"Slam_Lost",
	
	"PlayArea_FenceVisible",

	"Recenter",

	"SET_ROOMSETUP_SIMPLE_MODE_HEIGHTS",
	"GET_ROOMSETUP_SIMPLE_MODE_HEIGHTS",
	"SET_ROOMSETUP_SIMPLE_MODE_HEIGHTS_RESULT",

	"RESET_SIMPLE_MOME_ROOMSETUP",

	"WRITE_DEVICE_SLEEP_DURATION",

	"READ_DEVICE_SLEEP_DURATION",

	"UPDATE_ORIGIN",
};
