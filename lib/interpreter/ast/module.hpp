#pragma once
#include <lib/interpreter/ast/statement.hpp>
#include <lib/interpreter/ast/expression.hpp>
#include <lib/static.string.hpp>
#include <lib/concept.hpp>

namespace lib::interpreter {
    namespace ast {
        template<class ...Functions>
        struct Module
        {
            std::tuple<Functions...> functions;

            constexpr static inline auto fsize = sizeof...(Functions);
            constexpr static inline auto tsize = 0;
            constexpr static inline auto vsize = 0;

            constexpr explicit Module(const Functions& ...functions) noexcept
            : functions(functions...)
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

    template<class ...Functions>
    constexpr auto module(const Functions& ...functions) noexcept
    {
        return ast::Module<Functions...>(functions...);
    }
}
