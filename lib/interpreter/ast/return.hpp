#pragma once
#include <lib/interpreter/ast/statement.hpp>
#include <lib/interpreter/ast/expression.hpp>
#include <lib/concept.hpp>

namespace lib::interpreter {
    namespace ast {
        template<class RValue>
        struct Return: Statement
        {
            RValue expression;

            constexpr explicit Return(const RValue& expression) noexcept
            : expression(expression)
            {}
            constexpr Return(const Return& other) = default;
            constexpr Return& operator=(const Return& other) = default;

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };
    }

    template<class RValue, typename = lib::Require<std::is_base_of_v<ast::Expression, RValue>>>
    constexpr auto ret(const RValue& expression) noexcept
    {
        return ast::Return<RValue>(expression);
    }
}
