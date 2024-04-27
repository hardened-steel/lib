#pragma once

namespace lib {
    enum class Endian
    {
        Little = __ORDER_LITTLE_ENDIAN__,
        Big    = __ORDER_BIG_ENDIAN__,
        Native = __BYTE_ORDER__
    };
}
