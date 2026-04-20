/************************************************************************************
Filename    :
Content     :
Created     :   Feb , 2016
Authors     :   Tiehan Lu
Copyright   :   Copyright 2016 Deepoon, Inc. All Rights reserved.
************************************************************************************/
#pragma once

#define DPN_PROPERTY_IPD	"InterpupillaryDistance" // float
#define DPN_PROPERTY_FOV    "FieldOfViewX"  // float
#define DPN_PROPERTY_VIEWPORT_LEFT "ViewportX" // int
#define DPN_PROPERTY_VIEWPORT_TOP  "ViewportY" // int
#define DPN_PROPERTY_VIEWPORT_WIDTH	"ViewportWidth" // int
#define DPN_PROPERTY_VIEWPORT_HEIGHT "ViewportHeight" // int
#define DPN_PROPERTY_EDITOR_MODE  "EditorMode" // int
#define DPN_PROPERTY_RENDER_FLIP "RenderFlip" // int
#define DPN_PROPERTY_RENDER_TARGET_WIDTH "RenderTargetWidth"
#define DPN_PROPERTY_RENDER_TARGET_HEIGHT "RenderTargetHeight"
#define DPN_PROPERTY_RECENTER "Recenter" // int, take effect on only caller app
#define DPN_PROPERTY_RECENTER_GLOBAL "Recenter_Global" //int, take effect on all apps
#define DPN_PROPERTY_REFRESH_RATE "refresh_rate"
#define DPN_PROPERTY_FIX_POSE     "fix_pose"  // int, take effect on all aps
#define DPN_PROPERTY_SCREEN_OUTPUT  "pcScreenOutput"
#define DPN_PROPERTY_DUMP_ATW     "dump_atw"
#define DPN_PROPERTY_SET_YAW	 "set_yaw"
#define DPN_PROPERTY_BACK_LIGHT  "back_light"

#define DPN_PROPERTY_FIELD_OF_VIEW_LEFT_DEGREES     "field_of_view_left_degrees"
#define DPN_PROPERTY_FIELD_OF_VIEW_RIGHT_DEGREES    "field_of_view_right_degrees"
#define DPN_PROPERTY_FIELD_OF_VIEW_TOP_DEGREES      "field_of_view_top_degrees"
#define DPN_PROPERTY_FIELD_OF_VIEW_BOTTOM_DEGREES   "field_of_view_bottom_degrees"
#define DPN_PROPERTY_TRACKING_RANGE_MINIMUM_METERS  "tracking_range_minimum_meters"
#define DPN_PROPERTY_TRACKING_RANGE_MAXIMUM_METERS  "tracking_range_maximum_meters"

#define DPN_PROPERTY_MIRROR "MirrorMode"
#define DPN_PROPERTY_USER_ID "UserID"
#define DPN_PROPERTY_HMD_DEVICE_ID "HmdDeviceID"
#define DPN_PROPERTY_HMD_TICKET		"Ticket"
#define DPN_PROPERTY_WIRELESS_FLAG "WirelessFlag"

// game engine properties
#define DPN_PROPERTY_GAME_ENGINE_TYPE "game_engine_type" // game engine type
#define DPN_PROPERTY_GAME_ENGINE_COLOR_SPACE "game_engine_color_space"  // game engine color space
