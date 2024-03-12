set(PCommon 0)
set(FreeRTOS 1)
set(Linux 2)
set(Windows 3)
set(Darwin 4)
set(Android 5)
set(FreeBSD 6)
set(CrayLinuxEnvironment 7)
set(MSYS 8)

if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    set(PLATFORM ${FreeRTOS})
    set(PLATFORM_NAME "freertos")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM ${Linux})
    set(PLATFORM_NAME "linux")
    set(PCommon ${PLATFORM})
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM ${Windows})
    set(PLATFORM_NAME "windows")
    set(PCommon ${PLATFORM})
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM ${Darwin})
    set(PLATFORM_NAME "darwin")
    set(PCommon ${PLATFORM})
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(PLATFORM ${Android})
    set(PLATFORM_NAME "android")
    set(PCommon ${PLATFORM})
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(PLATFORM_NAME "freebsd")
    set(PLATFORM ${FreeBSD})
    set(PCommon ${PLATFORM})
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "CrayLinuxEnvironment")
    set(PLATFORM ${CrayLinuxEnvironment})
    set(PLATFORM_NAME "craylinuxenv")
    set(PCommon ${PLATFORM})
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    set(PLATFORM ${MSYS})
    set(PLATFORM_NAME "msys")
    set(PCommon ${PLATFORM})
endif()

add_compile_definitions(PLATFORM=${PLATFORM}
    FreeRTOS=${FreeRTOS}
    Linux=${Linux}
    Windows=${Windows}
    Darwin=${Darwin}
    Android=${Android}
    FreeBSD=${FreeBSD}
    CrayLinuxEnvironment=${CrayLinuxEnvironment}
    MSYS=${MSYS}
    COMMON=${PCommon}
    SYSTEM_NAME=${CMAKE_SYSTEM}
)
