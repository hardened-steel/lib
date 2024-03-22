#pragma once
#include <lib/interpreter/ast/statement.hpp>
#include <lib/static.string.hpp>
#include <lib/concept.hpp>

namespace lib::interpreter {
    namespace ast {
        template<class ...Statements>
        struct Module
        {
            std::tuple<Statements...> statements;

            constexpr static inline auto size = sizeof...(Statements);

            constexpr explicit Module(const Statements& ...statements) noexcept
            : statements(statements...)
            {}
            constexpr Module(const Module& other) = default;
            constexpr Module& operator=(const Module& other) = default;

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };
    }

    template<class ...Statements>
    constexpr auto module(const Statements& ...statements) noexcept
    {
        return ast::Module<Statements...>(statements...);
    }
}
