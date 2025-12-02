#pragma once
#include <ratio>
#include <lib/typetraits/if.hpp>

namespace lib {
    template <class ...Ratios>
    struct RatioMultiplyF;

    template <class ...Ratios>
    using RatioMultiply = typename RatioMultiplyF<Ratios...>::Result;

    template <class Ratio, class ...Ratios>
    struct RatioMultiplyF<Ratio, Ratios...>
    {
        using Result = std::ratio_multiply<Ratio, RatioMultiply<Ratios...>>;
    };

    template <class Ratio>
    struct RatioMultiplyF<Ratio>
    {
        using Result = Ratio;
    };

    template <class Ratio, int P>
    struct RatioPowF
    {
        constexpr static inline auto degree = (P < 0) ? -P : +P;
        using Pow = std::ratio_multiply<Ratio, typename RatioPowF<Ratio, degree - 1>::Result>;
        using Result = typetraits::If <
            P < 0,
            std::ratio_divide, typetraits::List<std::ratio<1>, Pow>,
            typetraits::Constant, typetraits::List<Pow>
        >;
    };

    template <class Ratio>
    struct RatioPowF<Ratio, 1>
    {
        using Result = Ratio;
    };

    template <class Ratio>
    struct RatioPowF<Ratio, 0>
    {
        using Result = std::ratio<1>;
    };

    template <class Ratio, int P>
    using RatioPow = typename RatioPowF<Ratio, P>::Result;
}
