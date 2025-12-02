#pragma once
#include <cstddef>


namespace lib {
    template <class T, std::size_t N>
    using RawArray = T(&)[N]; // NOLINT
}
