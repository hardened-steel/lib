#pragma once
#include <tuple>
#include <lib/interpreter/ast/statement.hpp>
#include <lib/typetraits/set.hpp>

namespace lib::interpreter {
    namespace ast {
        template<class T>
        struct FilterVariableDeclarationF
        {
            using Result = void;
        };

        template<std::size_t N, class InitExpression>
        struct FilterVariableDeclarationF<VariableDeclaration<N, InitExpression>>
        {
            using Result = VariableDeclaration<N, InitExpression>;
        };

        template<class T>
        using FilterVariableDeclaration = typename FilterVariableDeclarationF<T>::Result;

        template<class ...Statements>
        using GetVariables = lib::typetraits::Erase<
            lib::typetraits::Apply<
                lib::typetraits::CreatetSet<Statements...>,
                FilterVariableDeclaration
            >,
            void
        >;

        template<class ...Statements>
        struct Scope: Statement
        {
            std::tuple<Statements...> statemets;

            constexpr explicit Scope(const Statements& ...statemets) noexcept
            : statemets(statemets...)
            {}
            constexpr explicit Scope(const std::tuple<Statements...>& statemets) noexcept
            : statemets(statemets)
            {}
            constexpr Scope(const Scope& other) = default;
            constexpr Scope& operator=(const Scope& other) = default;

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };
    }

    template<class ...Statements>
    constexpr auto scope(const Statements& ...statements) noexcept
    {
        return ast::Scope<Statements...>(statements...);
    }
}
