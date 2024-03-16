#pragma once
#include <lib/interpreter/ast/statement.hpp>
#include <lib/interpreter/ast/expression.hpp>
#include <lib/typetraits/set.hpp>
#include <lib/static.string.hpp>
#include <lib/concept.hpp>

namespace lib::interpreter {
    namespace ast {
        template<class InitExpression>
        struct VariableDeclaration: Statement
        {
            std::string_view name;
            InitExpression init;

            constexpr explicit VariableDeclaration(std::string_view name, const InitExpression& init) noexcept
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

        struct Variable: Expression
        {
            std::string_view name;

            constexpr explicit Variable(std::string_view name) noexcept
            : name(name)
            {}
            constexpr Variable(const Variable& other) = default;
            constexpr Variable& operator=(const Variable& other) = default;

            template<class RValue, typename = lib::Require<std::is_base_of_v<Expression, RValue>>>
            constexpr auto operator=(const RValue& rhs) const noexcept
            {
                return VariableDeclaration<RValue>(name, rhs);
            }

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        struct Var
        {
            constexpr auto operator()(std::string_view name) const noexcept
            {
                return Variable(name);
            }
        };
    }

    constexpr inline ast::Var var;
}
