#pragma once
#include <numeric>

namespace lib {
    template <class Lhs, class Rhs>
    constexpr auto gcd(Lhs lhs, Rhs rhs) noexcept
    {
        return std::gcd(lhs, rhs);
    }

    template <class T, class ...TArgs>
    constexpr auto gcd(T arg, TArgs ...args) noexcept
    {
        return gcd(arg, gcd(args...));
    }

    template <class T>
    constexpr auto gcd(T arg) noexcept
    {
        return arg;
    }

    template <class Lhs, class Rhs>
    constexpr auto lcm(Lhs lhs, Rhs rhs) noexcept
    {
        return std::lcm(lhs, rhs);
    }

    template <class T, class ...TArgs>
    constexpr auto lcm(T arg, TArgs ...args) noexcept
    {
        return lcm(arg, lcm(args...));
    }

    template <class T>
    constexpr auto lcm(T arg) noexcept
    {
        return arg;
    }
}
