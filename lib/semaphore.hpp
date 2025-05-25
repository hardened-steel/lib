#pragma once
#include <lib/platform.hpp>

#if LIB_PLATFORM == LIB_PLATFORM_WINDOWS
#   include <lib/platform/windows/semaphore.hpp>
#endif
#if LIB_PLATFORM == LIB_PLATFORM_LINUX
#   include <lib/platform/linux/semaphore.hpp>
#endif
