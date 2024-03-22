#pragma once
#include <lib/interpreter/ast/expression.hpp>
#include <lib/concept.hpp>

namespace lib::interpreter::ast {
    template<class Lhs, class Rhs>
    struct Sum: Expression
    {
        Lhs lhs;
        Rhs rhs;

        constexpr explicit Sum(const Lhs& lhs, const Rhs& rhs) noexcept
        : lhs(lhs), rhs(rhs)
        {}
        constexpr Sum(const Sum& other) = default;
        constexpr Sum& operator=(const Sum& other) = default;

        template<class Context, class ...TArgs>
        constexpr auto operator()(Context& ctx, TArgs&& ...args) const
        {
            return ctx(*this, std::forward<TArgs>(args)...);
        }
    };

    template<
        class Lhs, class Rhs,
        typename = lib::Require<std::is_base_of_v<ast::Expression, Lhs>, std::is_base_of_v<ast::Expression, Rhs>>
    >
    constexpr auto operator+(const Lhs& lhs, const Rhs& rhs) noexcept
    {
        return Sum<Lhs, Rhs>(lhs, rhs);
    }
}
