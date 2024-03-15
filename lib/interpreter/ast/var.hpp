#pragma once
#include <lib/interpreter/ast/statement.hpp>
#include <lib/interpreter/ast/expression.hpp>
#include <lib/typetraits/set.hpp>
#include <lib/static.string.hpp>
#include <lib/concept.hpp>

namespace lib::interpreter {
    namespace ast {
        template<std::size_t N, class InitExpression>
        struct VariableDeclaration: Statement
        {
            lib::StaticString<N> name;
            InitExpression init;

            constexpr explicit VariableDeclaration(lib::StaticString<N> name, const InitExpression& init) noexcept
            : name(name), init(init)
            {}
            constexpr VariableDeclaration(const VariableDeclaration& other) = default;
            constexpr VariableDeclaration& operator=(const VariableDeclaration& other) = default;

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        template<std::size_t N>
        struct Variable: Expression
        {
            lib::StaticString<N> name;

            constexpr explicit Variable(lib::StaticString<N> name) noexcept
            : name(name)
            {}
            constexpr Variable(const Variable& other) = default;
            constexpr Variable& operator=(const Variable& other) = default;

            template<class RValue, typename = lib::Require<std::is_base_of_v<Expression, RValue>>>
            constexpr auto operator=(const RValue& rhs) const noexcept
            {
                return VariableDeclaration<N, RValue>(name, rhs);
            }

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        struct Var
        {
            template<std::size_t N>
            constexpr auto operator()(const char (&name)[N]) const noexcept
            {
                return Variable<N - 1>(name);
            }
        };
    }

    constexpr inline ast::Var var;
}
