#pragma once
#include <tuple>
#include <lib/interpreter/ast/operator.hpp>
#include <lib/typetraits/set.hpp>

namespace lib::interpreter {
    namespace ast {
        template<class T>
        struct FilterVariableDeclarationF
        {
            using Result = void;
        };

        template<class InitExpression>
        struct FilterVariableDeclarationF<VariableDeclaration<InitExpression>>
        {
            using Result = VariableDeclaration<InitExpression>;
        };

        template<class T>
        using FilterVariableDeclaration = typename FilterVariableDeclarationF<T>::Result;

        template<class ...Operators>
        using GetVariables = lib::typetraits::Erase<
            lib::typetraits::Apply<
                lib::typetraits::CreatetSet<Operators...>,
                FilterVariableDeclaration
            >,
            void
        >;

        template<class ...Operators>
        struct Scope: Operator
        {
            std::tuple<Operators...> operators;

            constexpr static inline auto size = sizeof...(Operators);

            constexpr explicit Scope(const Operators& ...operators) noexcept
            : operators(operators...)
            {}
            constexpr explicit Scope(const std::tuple<Operators...>& operators) noexcept
            : operators(operators)
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

    template<class ...Operators>
    constexpr auto scope(const Operators& ...operators) noexcept
    {
        return ast::Scope<Operators...>(operators...);
    }
}
