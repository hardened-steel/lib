#pragma once
#if PLATFORM == Windows
#   define LIB_UNREACHABLE __assume(0)
#endif
#if PLATFORM == Linux
#   define LIB_UNREACHABLE __builtin_unreachable()
#endif
