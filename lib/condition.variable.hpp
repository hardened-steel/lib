#pragma once
#if PLATFORM == FreeRTOS
#   include "freertos/condition.variable.hpp"
#endif
#if PLATFORM == COMMON
#   include "common/condition.variable.hpp"
#endif
