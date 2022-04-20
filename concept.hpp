#pragma once
#include <type_traits>

namespace lib {
    template<bool ...Conditions>
    using Require = std::enable_if_t<(Conditions && ...), void>;
}