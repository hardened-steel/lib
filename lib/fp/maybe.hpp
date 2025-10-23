#pragma once
#include <lib/fp/function.hpp>
#include <lib/fp/map.hpp>
#include <lib/typetraits/map.hpp>
#include <lib/typetraits/if.hpp>


namespace lib::fp {
    template <class T>
    using Maybe = Val<std::optional<T>>;

    template <class From, class FImpl, class ...Ts>
    Maybe<std::invoke_result_t<Map, Fn<From, Ts...>, From>> do_map(Fn<std::optional<From>, FImpl> input, Fn<From, Ts...> convertor)
    {
        if (auto value = co_await input) {
            co_return map(input, convertor);
        }
        co_return std::nullopt;
    }

    template <class From, class To, class TImpl>
    Maybe<To> do_map(std::nullopt_t, Fn<From, To, TImpl>)
    {
        return Maybe<To>(std::nullopt);
    }

    template <template <class> class T>
    struct TypeClass {};

    template <class T>
    Maybe<std::remove_cvref_t<T>> pure(T&& value, TypeClass<Maybe>)
    {
        return std::forward<T>(value);
    }

    template <class Lhs, class Rhs>
    Maybe<std::invoke_result_t<Map, Lhs, Rhs>> lift_a2(Maybe<Lhs> lhs, Maybe<Rhs> rhs)
    {
        auto l_value = co_await lhs;
        auto r_value = co_await rhs;
        if (l_value && r_value) {
            co_return map(l_value, r_value);
        }
        co_return std::nullopt;
    }

    unittest {
        SimpleExecutor executor;

        int (*do_half)(int) = [] (int value) noexcept {
            return value / 2;
        };

        const Fn half = do_half;
        //check((std::make_optional(1) | half)(executor) == 0);
        //check((std::make_optional(2) | half)(executor) == 1);
        //check((std::nullopt | half)(executor) == std::nullopt);
    }

    template <class T>
    struct Eq
    {
        static inline const Fn<T, T, bool> compare;
    };

    template <class T>
    struct Eq<std::optional<T>>
    {
        static bool do_compare(std::optional<T> lhs, std::optional<T> rhs) noexcept
        {
            return lhs == rhs;
        }
        static inline const Fn compare = do_compare;
    };

    template <StaticString Name, class ...Args>
    struct Type {};

    template <class ...Args>
    struct Signature {};

    template <template <class ...> class T>
    struct Template {};

    template <class T>
    struct JustType {};

    template <class T, class Map, class Found = void>
    struct ReplaceF;

    template <StaticString Name, class ...Args, class Map>
    struct ReplaceF<Type<Name, Args...>, Map, void>
    {
        using Result = typename ReplaceF<
            Type<Name, Args...>, Map,
            typetraits::VTag<typetraits::exists<Map, Type<Name>>, typetraits::exists<Map, Type<Name, Args...>>>
        >::Result;
    };

    template <template <class ...> class U, class ...Args, class Map>
    struct ReplaceF<U<Args...>, Map, void>
    {
        using Result = U<typename ReplaceF<Args, Map>::Result...>;
    };

    template <StaticString Name, class ...Args, class Map>
    struct ReplaceF<Type<Name, Args...>, Map, typetraits::VTag<false, false>>
    {
        using Result = Type<Name,
            typename ReplaceF<Args, Map>::Result...
        >;
    };

    template <StaticString Name, class ...Args, class Map>
    struct ReplaceF<Type<Name, Args...>, Map, typetraits::VTag<true, false>>
    {
        using Result = typename ReplaceF<
            Type<Name, Args...>, Map,
            typetraits::Get<Map, typetraits::index<Map, Type<Name>>>
        >::Result;
    };

    template <StaticString Name, class ...Args, class Map>
    struct ReplaceF<Type<Name, Args...>, Map, typetraits::VTag<true, true>>
    {
        using Result = typename ReplaceF<Type<Name>, Map, typetraits::Get<Map, typetraits::index<Map, Type<Name>>>>::Result;
    };

    template <class T, class Map, class U>
    struct ReplaceF<T, Map, JustType<U>>
    {
        using Result = U;
    };

    template <class T, class Map>
    struct ReplaceF<T, Map, void>
    {
        using Result = T;
    };

    template <StaticString Name, class ...Args, class Map, template <class ...> class U>
    struct ReplaceF<Type<Name, Args...>, Map, Template<U>>
    {
        using Result = U<typename ReplaceF<Args, Map>::Result...>;
    };

    template <class T, class Map>
    using Replace = typename ReplaceF<T, Map>::Result;


    template <class Signature, class T, class Map>
    struct CallF;

    template <class Signature, class T>
    using Call = typename CallF<Signature, T, typetraits::Map<>>::Result;

    template <StaticString Name, class ...Others, class T, class Map>
    struct CallF<typetraits::List<Type<Name>, Others...>, T, Map>
    {
        using Result = typetraits::Map<typetraits::Map<Type<Name>, JustType<T>>>;
    };

    template <StaticString Name, class ...IArgs, class ...Others, template <class...> class T, class ...TArgs, class Map>
    struct CallF<typetraits::List<Type<Name, IArgs...>, Others...>, T<TArgs...>, Map>
    {
        using Result = typetraits::Merge<
            typetraits::Map<typetraits::Map<Type<Name>, Template<T>>>,
            typename CallF<typetraits::List<IArgs...>, TArgs, typetraits::Map<>>::Result ...
        >;
    };

    template <class ...Ts, class ...Others, class ...TArgs, class Map>
    struct CallF<typetraits::List<Fn<Ts...>, Others...>, Fn<TArgs...>, Map>
    {
        using Result = typename CallF<Fn<Ts...>, typetraits::PopBack<typetraits::List<TArgs...>>, Map>::Result;
    };

    template <class ...Ts, class ...TArgs, class Map>
    struct CallF<Fn<Ts...>, typetraits::List<TArgs...>, Map>
    {
        using Result = typetraits::Merge<
            Map,
            typename CallF<typetraits::List<Ts>, TArgs, typetraits::Map<>>::Result ...
        >;
    };

    template <StaticString Name, class ...IArgs, class ...Others, class T, class Map>
    struct CallF<typetraits::List<Type<Name, IArgs...>, Others...>, T, Map>
    {
        static_assert(false, "wrong argument");
    };


    template <class Signature, class Map>
    struct ApplyF;

    template <StaticString Name, class ...TArgs, class ...Others, class Map>
    struct ApplyF<typetraits::List<Type<Name, TArgs...>, Others...>, Map>
    {
        using Result = typetraits::Concat<
            typetraits::List<Replace<Type<Name, TArgs...>, Map>>,
            typename ApplyF<typetraits::List<Others>, Map>::Result...
        >;
    };

    template <class ...TArgs, class ...Others, class Map>
    struct ApplyF<typetraits::List<Fn<TArgs...>, Others...>, Map>
    {
        using Result = typetraits::Concat<
            typetraits::List<Fn<Replace<TArgs, Map>...>>,
            typename ApplyF<typetraits::List<Others>, Map>::Result...
        >;
    };

    template <class Signature, class Map>
    using Apply = typename ApplyF<Signature, Map>::Result;


    using Function1 = typetraits::List<Type<"A">, Type<"A">>;
    using Function2 = typetraits::List<Type<"A", Type<"B">>, Type<"A", float>, Type<"B">>;
    using Function3 = typetraits::List<Fn<Type<"M", Type<"C">>, Type<"B">>, Type<"M", Type<"C">>, Type<"M", Type<"B">>, Fn<Type<"B">>>;
  
    using C1 = Call<Function1, int>;
    using C2 = Call<Function2, std::optional<int>>;
    using C3 = Call<Function3, Fn<std::optional<int>, float, Dynamic>>;

    using A1 = Apply<Function1, C1>;
    using A2 = Apply<Function2, C2>;
    using A3 = Apply<Function3, C3>;

    namespace details {
        using typetraits::VTag;
        using typetraits::TTag;
        using typetraits::List;
        using typetraits::If;

        template <class>
        struct Empty;

        template <std::size_t It>
        struct Empty<VTag<It>>
        {
            constexpr static inline bool result = false;
            using Result = void;
            using Next = VTag<It>;
        };

        template <class Str, class It>
        struct Read;

        template <StaticString Str, std::size_t It>
        struct Read<VTag<Str>, VTag<It>>
        {
            constexpr static inline bool result = true;
            using Result = VTag<StaticString(std::array {Str[It]})>;
            using Next = VTag<It + 1>;
        };

        template <class Str, class It, class Condition>
        struct ParseSymbol;

        template <StaticString Str, std::size_t It, auto Condition>
        struct ParseSymbol<VTag<Str>, VTag<It>, VTag<Condition>>
        {
            constexpr static inline bool result = (It < Str.size()) && (Condition(Str[It]));
            using Type = If<
                result,
                Read, List<VTag<Str>, VTag<It>>,
                Empty, List<VTag<It>>
            >;
            using Result = typename Type::Result;
            using Next = typename Type::Next;
        };

        constexpr auto is_identifier_symbol = [] (char symbol) noexcept {
            return ((symbol >= 'a') && (symbol <= 'z')) || ((symbol >= 'A') && (symbol <= 'Z')) || (symbol == '_');
        };

        constexpr auto is_number = [] (char symbol) noexcept {
            return (symbol >= '0') && (symbol <= '9');
        };

        constexpr auto is_identifier_or_number_symbol = [] (char symbol) noexcept {
            return is_identifier_symbol(symbol) || is_number(symbol);
        };

        constexpr auto is_ws_symbol = [] (char symbol) noexcept {
            return (symbol == ' ') || (symbol == '\t') || (symbol == '\n');
        };

        template <class Lhs, class Rhs>
        struct Concat;

        template <StaticString Lhs, StaticString Rhs>
        struct Concat<VTag<Lhs>, VTag<Rhs>>
        {
            using Result = VTag<Lhs + Rhs>;
        };

        template <class Lhs, class Rhs>
        struct Append
        {
            using Result = List<Lhs, Rhs>;
        };

        template <class Lhs>
        struct Append<Lhs, void>
        {
            using Result = List<Lhs>;
        };

        template <class Rhs>
        struct Append<void, Rhs>
        {
            using Result = List<Rhs>;
        };

        template <>
        struct Append<void, void>
        {
            using Result = void;
        };

        template <class ...Lhs>
        struct Append<List<Lhs...>, void>
        {
            using Result = List<Lhs...>;
        };

        template <class ...Rhs>
        struct Append<void, List<Rhs...>>
        {
            using Result = List<Rhs...>;
        };

        template <class ...Lhs, class ...Rhs>
        struct Append<List<Lhs...>, List<Rhs...>>
        {
            using Result = List<Lhs..., Rhs...>;
        };

        template <class Lhs, class ...Rhs>
        struct Append<Lhs, List<Rhs...>>
        {
            using Result = List<Lhs, Rhs...>;
        };

        template <class ...Lhs, class Rhs>
        struct Append<List<Lhs...>, Rhs>
        {
            using Result = List<Lhs..., Rhs>;
        };

        template <template <class, class> class Parser>
        struct ParserCB {};

        template <class Str, class It, class Parser>
        struct Ignore;

        template <class Str, class It, template <class, class> class Parser>
        struct Ignore<Str, It, ParserCB<Parser>>: Parser<Str, It>
        {
            using Result = void;
        };

        template <class Str, class It, class Parser>
        struct ParseAnyTimes;

        template <StaticString Str, std::size_t It, template <class, class> class Parser>
        struct ParseAnyTimes<VTag<Str>, VTag<It>, ParserCB<Parser>>
        {
            using Head = Parser<VTag<Str>, VTag<It>>;
            using Tail = If<
                Head::result,
                ParseAnyTimes, List<VTag<Str>, typename Head::Next, ParserCB<Parser>>,
                Empty, List<VTag<It>>
            >;

            constexpr static inline bool result = true;
            using Result = std::conditional_t<
                Tail::result,
                typename Append<typename Head::Result, typename Tail::Result>::Result,
                std::conditional_t<
                    Head::result,
                    typename Head::Result,
                    void
                >
            >;
            using Next = typename Tail::Next;
        };


        template <class Str, class It, class Parser>
        struct ParseOptional;

        template <StaticString Str, std::size_t It, template <class, class> class Parser>
        struct ParseOptional<VTag<Str>, VTag<It>, ParserCB<Parser>>
        {
            using PResult = Parser<VTag<Str>, VTag<It>>;

            constexpr static inline bool result = true;
            using Result = std::conditional_t<PResult::result, typename PResult::Result, void>;
            using Next = std::conditional_t<PResult::result, typename PResult::Next, VTag<It>>;
        };


        template <class Str, class It, class Parsers>
        struct ParseSequence;

        template <StaticString Str, std::size_t It, template <class, class> class Parser>
        struct ParseSequence<VTag<Str>, VTag<It>, List<ParserCB<Parser>>>: Parser<VTag<Str>, VTag<It>>
        {
        };

        template <StaticString Str, std::size_t It, template <class, class> class HParsers, class ...TParsers>
        struct ParseSequence<VTag<Str>, VTag<It>, List<ParserCB<HParsers>, TParsers...>>
        {
            using Head = HParsers<VTag<Str>, VTag<It>>;
            using Tail = If<
                Head::result,
                ParseSequence, List<VTag<Str>, typename Head::Next, List<TParsers...>>,
                Empty, List<typename Head::Next>
            >;

            constexpr static inline bool result = Head::result && Tail::result;
            using Result = typename Append<typename Head::Result, typename Tail::Result>::Result;
            using Next = typename Tail::Next;
        };

        template <class Str, class It, class Parsers>
        struct ParseAlternative;

        template <StaticString Str, std::size_t It, template <class, class> class Parser>
        struct ParseAlternative<VTag<Str>, VTag<It>, List<ParserCB<Parser>>>: Parser<VTag<Str>, VTag<It>>
        {
        };

        template <StaticString Str, std::size_t It, template <class, class> class HParsers, class ...TParsers>
        struct ParseAlternative<VTag<Str>, VTag<It>, List<ParserCB<HParsers>, TParsers...>>
        {
            using Head = HParsers<VTag<Str>, VTag<It>>;
            using Tail = If<
                !Head::result,
                ParseAlternative, List<VTag<Str>, typename Head::Next, List<TParsers...>>,
                Empty, List<typename Head::Next>
            >;

            constexpr static inline bool result = Head::result || Tail::result;
            using Result = std::conditional_t<Head::result, typename Head::Result, typename Tail::Result>;
            using Next = std::conditional_t<Head::result, typename Head::Next, typename Tail::Next>;
        };


        template <class Str, class It>
        struct ParseWS;

        template <StaticString Str, std::size_t It>
        struct ParseWS<VTag<Str>, VTag<It>>
        {
            template <class IStr, class IIt>
            using ParseWSSymbols = ParseSymbol<IStr, IIt, VTag<is_ws_symbol>>;
            using WS = ParseAnyTimes<VTag<Str>, VTag<It>, ParserCB<ParseWSSymbols>>;

            constexpr static inline bool result = true;
            using Result = void;
            using Next = typename WS::Next;
        };

        template <class T>
        struct UnionStrings;

        template <StaticString ...Strs>
        struct UnionStrings<List<VTag<Strs>...>>
        {
            using Result = VTag<(Strs + ... + StaticString(""))>;
        };

        template <>
        struct UnionStrings<void>
        {
            using Result = VTag<StaticString("")>;
        };

        template <class Str, class It = VTag<std::size_t(0)>>
        struct ParseIdentifier;

        template <StaticString Str, std::size_t It>
        struct ParseIdentifier<VTag<Str>, VTag<It>>
        {
            template <class IStr, class NIt>
            using ParseFirstSymbol = ParseSymbol<IStr, NIt, VTag<is_identifier_symbol>>;

            template <class IStr, class NIt>
            using ParseNextSymbol = ParseSymbol<IStr, NIt, VTag<is_identifier_or_number_symbol>>;

            template <class IStr, class NIt>
            using ParseNextSymbols = ParseAnyTimes<IStr, NIt, ParserCB<ParseNextSymbol>>;

            using Sequence = ParseSequence<VTag<Str>, VTag<It>, List<ParserCB<ParseFirstSymbol>, ParserCB<ParseNextSymbols>>>;

            constexpr static inline bool result = Sequence::result;
            using Result = typename UnionStrings<typename Sequence::Result>::Result;
            using Next = typename Sequence::Next;
        };

        unittest {
            constexpr StaticString string = "hello1 world";
            using Foo = ParseIdentifier<VTag<string>>;
            constexpr bool result = Foo::result;
            static_assert(std::is_same_v<typename Foo::Result, VTag<StaticString("hello1")>>);
        }

        unittest {
            constexpr StaticString string = "";
            using Foo = ParseIdentifier<VTag<string>>;
            constexpr bool result = Foo::result;
            static_assert(!result);
        }

        template <class T>
        struct UnwrapType;

        template <StaticString Name>
        struct UnwrapType<List<VTag<Name>>>
        {
            using Result = Type<Name>;
        };

        template <StaticString Name, class ...Args>
        struct UnwrapType<List<Type<Name, Args...>>>
        {
            using Result = Type<Name, Args...>;
        };

        template <StaticString Name, class ...Args>
        struct UnwrapType<List<VTag<Name>, Args...>>
        {
            using Result = Type<Name, Args...>;
        };

        template <StaticString Name, StaticString Param, class ...Args>
        struct UnwrapType<List<VTag<Name>, Type<Param, Args...>>>
        {
            using Result = Type<Name, Type<Param, Args...>>;
        };

        template <class Str, class It = VTag<std::size_t(0)>>
        struct ParseType;

        template <class Str, class It = VTag<std::size_t(0)>>
        struct ParseFunction;

        template <StaticString Str, std::size_t It>
        struct ParseType<VTag<Str>, VTag<It>>
        {
            constexpr static inline auto is_obrace = [] (char symbol) noexcept {
                return symbol == '[';
            };

            constexpr static inline auto is_cbrace = [] (char symbol) noexcept {
                return symbol == ']';
            };

            template <class IStr, class NIt>
            using ParseOBrace = ParseSymbol<IStr, NIt, VTag<is_obrace>>;

            template <class IStr, class NIt>
            using IgnoreParseOBrace = Ignore<IStr, NIt, ParserCB<ParseOBrace>>;

            template <class IStr, class NIt>
            using ParseCBrace = ParseSymbol<IStr, NIt, VTag<is_cbrace>>;

            template <class IStr, class NIt>
            using IgnoreParseCBrace = Ignore<IStr, NIt, ParserCB<ParseCBrace>>;

            template <class IStr, class NIt>
            using ParseParameter = ParseSequence<IStr, NIt, List<ParserCB<ParseWS>, ParserCB<ParseType>>>;

            template <class IStr, class NIt>
            using ParseParameters = ParseAnyTimes<IStr, NIt, ParserCB<ParseParameter>>;

            template <class IStr, class NIt>
            using ParseParametersList = ParseSequence<
                IStr, NIt,
                List<
                    ParserCB<IgnoreParseOBrace>,
                    ParserCB<ParseParameters>,
                    ParserCB<IgnoreParseCBrace>
                >
            >;

            template <class IStr, class NIt>
            using OptionalParametersList = ParseOptional<IStr, NIt, ParserCB<ParseParametersList>>;

            using PResult = ParseSequence<
                VTag<Str>, VTag<It>,
                List<
                    ParserCB<ParseIdentifier>,
                    ParserCB<OptionalParametersList>
                >
            >;

            constexpr static inline bool result = PResult::result;
            using Result = typename UnwrapType<typename PResult::Result>::Result;
            using Next = typename PResult::Next;
        };

        unittest {
            constexpr StaticString string = "A[B C D[E]]";
            using Foo = ParseType<VTag<string>>;
            using Result = typename Foo::Result;
            using Next = typename Foo::Next;
            static_assert(std::is_same_v<Result, Type<"A", Type<"B">, Type<"C">, Type<"D", Type<"E">>>>);
        }


        template <class T>
        struct UnwrapFunction;

        template <StaticString Name, class ...Args>
        struct UnwrapFunction<List<Type<Name, Args...>>>
        {
            using Result = Signature<Type<Name, Args...>>;
        };

        template <class ...Ts>
        struct UnwrapFunction<List<Signature<Ts...>>>
        {
            using Result = Signature<Ts...>;
        };

        template <StaticString Name, class ...Args, class ...Ts>
        struct UnwrapFunction<List<Type<Name, Args...>, Signature<Ts...>>>
        {
            using Result = Signature<Type<Name, Args...>, Ts...>;
        };

        template <class ...Ts, StaticString Name, class ...Args>
        struct UnwrapFunction<List<Signature<Ts...>, Type<Name, Args...>>>
        {
            using Result = Signature<Ts..., Type<Name, Args...>>;
        };

        template <class ...Lhs, class ...Rhs>
        struct UnwrapFunction<List<Signature<Lhs...>, Signature<Rhs...>>>
        {
            using Result = Signature<Signature<Lhs...>, Rhs...>;
        };

        template <StaticString Str, std::size_t It>
        struct ParseFunction<VTag<Str>, VTag<It>>
        {
            constexpr static inline auto is_obrace = [] (char symbol) noexcept {
                return symbol == '(';
            };

            constexpr static inline auto is_cbrace = [] (char symbol) noexcept {
                return symbol == ')';
            };

            template <class IStr, class NIt>
            using ParseOBrace = ParseSymbol<IStr, NIt, VTag<is_obrace>>;

            template <class IStr, class NIt>
            using IgnoreParseOBrace = Ignore<IStr, NIt, ParserCB<ParseOBrace>>;

            template <class IStr, class NIt>
            using ParseCBrace = ParseSymbol<IStr, NIt, VTag<is_cbrace>>;

            template <class IStr, class NIt>
            using IgnoreParseCBrace = Ignore<IStr, NIt, ParserCB<ParseCBrace>>;

            constexpr static inline auto is_minus = [] (char symbol) noexcept {
                return symbol == '-';
            };

            constexpr static inline auto is_greater = [] (char symbol) noexcept {
                return symbol == '>';
            };

            template <class IStr, class NIt>
            using ParseMinus = ParseSymbol<IStr, NIt, VTag<is_minus>>;

            template <class IStr, class NIt>
            using ParseGreater = ParseSymbol<IStr, NIt, VTag<is_greater>>;

            template <class IStr, class NIt>
            using ParseArrow = ParseSequence<IStr, NIt, List<ParserCB<ParseMinus>, ParserCB<ParseGreater>>>;

            template <class IStr, class NIt>
            using IgnoreParseArrow = Ignore<IStr, NIt, ParserCB<ParseArrow>>;

            template <class IStr, class NIt>
            using ParseNextParameter = ParseSequence<
                IStr, NIt,
                List<
                    ParserCB<ParseWS>, ParserCB<IgnoreParseArrow>, ParserCB<ParseWS>, ParserCB<ParseFunction>
                >
            >;

            template <class IStr, class NIt>
            using OptionalParseNextParameter = ParseOptional<IStr, NIt, ParserCB<ParseNextParameter>>;

            template <class IStr, class NIt>
            using ParseWithoutBraces = ParseSequence<
                IStr, NIt,
                List<
                    ParserCB<ParseType>,
                    ParserCB<OptionalParseNextParameter>
                >
            >;

            template <class IStr, class NIt>
            using ParseWithBraces = ParseSequence<
                IStr, NIt,
                List<
                    ParserCB<IgnoreParseOBrace>,
                    ParserCB<ParseFunction>,
                    ParserCB<IgnoreParseCBrace>,
                    ParserCB<OptionalParseNextParameter>
                >
            >;

            using PResult = ParseAlternative<
                VTag<Str>, VTag<It>,
                List<
                    ParserCB<ParseWithBraces>,
                    ParserCB<ParseWithoutBraces>
                >
            >;

            constexpr static inline bool result = PResult::result;
            using Result = typename UnwrapFunction<typename PResult::Result>::Result;
            using Next = typename PResult::Next;
        };

        unittest {
            constexpr StaticString string = "a -> (c -> d)";
            using Foo = ParseFunction<VTag<string>>;
            using Next = typename Foo::Next;
            using Result = typename Foo::Result;
            constexpr auto result = Foo::result;
            static_assert(std::is_same_v<Result, Signature<Type<"a">, Type<"c">, Type<"d">>>);
        }

        unittest {
            constexpr StaticString string = "(a -> c) -> d";
            using Foo = ParseFunction<VTag<string>>;
            using Next = typename Foo::Next;
            using Result = typename Foo::Result;
            constexpr auto result = Foo::result;
            static_assert(
                std::is_same_v<
                    Result,
                    Signature<
                        Signature<
                            Type<"a">,
                            Type<"c">
                        >,
                        Type<"d">
                    >
                >
            );
        }

        unittest {
            constexpr StaticString string = "(A[B C] -> C) -> (B -> C) -> A[C]";
            using Foo = ParseFunction<VTag<string>>;
            using Next = typename Foo::Next;
            using Result = typename Foo::Result;
            constexpr auto result = Foo::result;
            static_assert(
                std::is_same_v<
                    Result,
                    Signature<
                        Signature<Type<"A", Type<"B">, Type<"C">>, Type<"C">>,
                        Signature<Type<"B">, Type<"C">>,
                        Type<"A", Type<"C">>
                    >
                >
            );
        }
    }
}
