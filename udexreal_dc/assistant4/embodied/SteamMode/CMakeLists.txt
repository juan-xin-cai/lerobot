cmake_minimum_required (VERSION 3.8)
project(SteamMode)

set(TARGET_NAME SteamMode)

# 头文件路径
include_directories(/opt/dpvr/assistant4/embodied/include)

file(GLOB_RECURSE source_files
    "*.h"
    "*.cpp"
    "*.c"
)

add_executable(${TARGET_NAME} ${source_files})

if(UNIX)
    # 基础系统库
    target_link_libraries(${TARGET_NAME} PRIVATE dl m rt)
    
    # DPVR SDK 库（绝对路径，位于 sdk/bin）
    target_link_libraries(${TARGET_NAME} PRIVATE
        /opt/dpvr/assistant4/sdk/bin/libDpvrPlatform_x64.so
        /opt/dpvr/assistant4/sdk/lib/libDpvrCommon_x64.so
        /opt/dpvr/assistant4/sdk/lib/libDpvrEliteDDK_x64.so
        # 如果还需要其他库，可以在这里添加
    )
endif()

if(INSTALL_SAMPLES)
    install(FILES ${source_files} DESTINATION ./samples/SteamMode)
endif()
