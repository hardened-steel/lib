#pragma once
#include <lib/interpreter/ast/expression.hpp>
#include <lib/literal.hpp>

namespace lib::interpreter {
    namespace ast {
        template<class T>
        struct Literal: Expression
        {
            T value;

            constexpr explicit Literal(T value) noexcept
            : value(value)
            {}
            constexpr Literal(const Literal& other) = default;
            constexpr Literal& operator=(const Literal& other) = default;

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };
    }

    template<char ...Chars>
    constexpr auto operator ""_l() noexcept
    {
        using Parser = lib::literal::Parser<Chars...>;
        return ast::Literal<typename Parser::Type>(Parser::value);
    }

    template<class T>
    constexpr auto literal(const T& value) noexcept
    {
        return ast::Literal<T>(value);
    }
}
