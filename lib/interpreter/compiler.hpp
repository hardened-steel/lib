#pragma once
#include <lib/interpreter/core.hpp>

namespace lib::interpreter {
    struct Type
    {
        const char* name;
    };

    constexpr auto operator""_t(const char* str, std::size_t) noexcept
    {
        return Type{str};
    }

    struct Name
    {
        const char* name;
    };

    struct Overload
    {

    };

    struct TypeInfo
    {
        Name          name;
        std::uint32_t size;
        //lib::span<>   constructors;
        //    destructor;
        //Fields        fields;
    };


    struct OperatorPlus
    {
        Name lhs;
        Name rhs;
    };

    constexpr auto operator+(const Name& lhs, const Name& rhs) noexcept
    {
        return OperatorPlus{lhs, rhs};
    }

    constexpr auto operator""_n(const char* str, std::size_t) noexcept
    {
        return Name{str};
    }

    struct Var
    {
        Type type;
        Name name;
    };

    template<class ...Statemets>
    constexpr auto body(Statemets ...statemets) noexcept
    {
        return std::make_tuple(statemets...);
    }

    struct FunctionPrototype
    {
        Name                  name;
        lib::span<const Var>  params;
        lib::span<const Type> ret;
    };

    template<class Body>
    struct Function
    {
        FunctionPrototype prototype;
        Body body;
    };

    template<class Body>
    constexpr auto function(Name name, lib::span<const Var> params, lib::span<const Type> ret, Body body) noexcept
    {
        return Function<Body>{FunctionPrototype{name, params, ret}, body};
    }

    template<class Body>
    constexpr auto compile(const Function<Body>& function)
    {
        return compile(function.body);
    }

    template<class ...Statemets, std::size_t ...I>
    constexpr auto compile(const std::tuple<Statemets...>& body, std::index_sequence<I...>) noexcept
    {
        return concat(compile(std::get<I>(body))...);
    }

    template<class ...Statemets>
    constexpr auto compile(const std::tuple<Statemets...>& body) noexcept
    {
        return compile(body, std::make_index_sequence<sizeof...(Statemets)>{});
    }

    constexpr auto compile(const OperatorPlus& plus) noexcept
    {
        return 1;
    }

    struct ErrorResult
    {
        std::string_view message;
    };

    template<class T>
    struct SuccessResult
    {
        T value;
    };

    template<class T, class Symbols, std::size_t It, std::size_t ...I>
    constexpr auto match(std::index_sequence<I...>)
    {
        if constexpr (((T::text[It + I] == Symbols::text[I]) && ...)) {
            return SuccessResult<std::string_view>{Symbols::text};
        } else {
            return ErrorResult{"not mathed"};
        }
    }

    template<class T, class Symbols, std::size_t It = 0>
    constexpr auto match()
    {
        return match<T, Symbols, It>(std::make_index_sequence<Symbols::text.size()>{});
    }
}
