#pragma once

#define LIB_PLATFORM_LINUX 1
#define LIB_PLATFORM_WINDOWS 2

#if __linux__
#   define LIB_PLATFORM LIB_PLATFORM_LINUX
#   define LIB_PLATFORM_NAME "linux"
#endif
#if _WIN32
#   define LIB_PLATFORM LIB_PLATFORM_WINDOWS
#   define LIB_PLATFORM_NAME "windows"
#endif
