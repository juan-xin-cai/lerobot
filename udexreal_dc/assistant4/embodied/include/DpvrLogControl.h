#pragma once
// handle control command
typedef enum {
    SDK_CONTROL_COMMAND_TYPE_SET_DDK_LOG_FILE = 1,
    SDK_CONTROL_COMMAND_TYPE_SET_DDK_LOG_MASK,

    SDK_CONTROL_COMMAND_TYPE_SET_STEAM_LOG_MASK = 0x10,

    SDK_CONTROL_COMMAND_TYPE_SET_SDK_LOG_MASK = 0x20
} SDK_CONTROL_COMMAND_TYPE;

#pragma warning(disable: 4200)  
typedef struct
{
    SDK_CONTROL_COMMAND_TYPE command;
    int user_data;
    int buf_len;
    char data[0];
} SDK_CONTROL_COMMAND;
#pragma warning(default: 4200)  
