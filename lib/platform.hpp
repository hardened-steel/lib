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

#define MSVC  1
#define CLANG 2
#define GCC   3

#ifdef _MSC_VER
#   define LIB_HOST_COMPILER MSVC
#endif
#ifdef __CLANG__
#   define LIB_HOST_COMPILER CLANG
#endif
#if defined(__GNUC__) && !defined(__CLANG__)
#   define LIB_HOST_COMPILER GCC
#endif
