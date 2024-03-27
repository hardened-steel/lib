#include <gtest/gtest.h>
#include <lib/interpreter/constexpr.hpp>
#include <lib/interpreter/compiler.hpp>

struct MyModule
{
    constexpr static auto import() noexcept
    {
        using namespace lib::interpreter;
        return lib::interpreter::module(
            /*fn("void", "u32::constructor")(param("u32", "this"), param("u32", "other"))(
                [](auto& context, const_expr::Pointer sp) noexcept {
                    auto& memory = context.memory;
                    const auto ptr = memory.load(context["this"], lib::tag<const_expr::Pointer>);
                    const auto other = memory.load(context["other"], lib::tag<std::uint32_t>);
                    const auto val = memory.load(other, lib::tag<std::uint32_t>);
                    memory.save(ptr, val);
                }
            ),
            fn("u32", "operator: lhs + rhs")(param("u32", "lhs"), param("u32", "rhs"))(
                [](auto& context, const_expr::Pointer sp) noexcept {
                    auto& memory = context.memory;
                    const auto lhs = memory.load(context["lhs"], lib::tag<std::uint32_t>);
                    const auto rhs = memory.load(context["rhs"], lib::tag<std::uint32_t>);
                    memory.save(context["return::value"], lhs + rhs);
                }
            ),
            fn("u32", "foo")(param("u32", "lhs"), param("u32", "rhs"))(
                [](auto& context, const_expr::Pointer sp) noexcept {
                    auto& memory = context.memory;
                    const auto lhs = memory.load(context["lhs"], lib::tag<std::uint32_t>);
                    const auto rhs = memory.load(context["rhs"], lib::tag<std::uint32_t>);
                    memory.save(context["return::value"], lhs + rhs);
                }
            ),*/
            fn("u32", "bar")(param("u32", "lhs"), param("u32", "rhs"))(
                //var("a") = fn("foo")(var("lhs"), 5_l),
                //var("b") = fn("foo")(var("lhs"), 5_l),
                //ret(var("a") + var("b") + 10_l)
                ret(var("lhs") + var("rhs"))
            )
        );
    }
};

enum Types: std::uint32_t
{
    U8,
    Ptr,
    StructBegin, StructEnd
};

struct Type0
{
    constexpr static inline std::array type {
        U8
    };
};

struct Type1
{
    constexpr static inline std::array type {
        Ptr, U8
    };
};

struct Type2
{
    constexpr static inline std::array type {
        StructBegin, U8, Ptr, U8, Ptr, StructBegin, U8, U8, StructEnd, U8, StructEnd
    };
};

struct Type3
{
    constexpr static inline std::array type {
        StructBegin, U8, Ptr, U8, Ptr, U8, StructEnd, StructBegin, U8, U8, StructEnd
    };
};

template<class T>
struct MakeStruct
{
    constexpr static inline auto type = lib::concat(std::array{StructBegin}, T::type);
};

template<Types ...types>
struct DecodeTypeF;

template<class T, class Indexes = void>
struct GetTypeF;

template<Types ...types>
using DecodeType = typename DecodeTypeF<types...>::Type;

template<Types ...types>
struct DecodeTypeF<U8, types...>
{
    using Type = std::uint8_t;
    struct Tail
    {
        constexpr static inline std::array<Types, sizeof...(types)> type {
            types...
        };
    };
};

template<Types ...types>
struct DecodeTypeF<Ptr, types...>
{
    using Decode = DecodeTypeF<types...>;
    using Type = typename Decode::Type*;
    using Tail = typename Decode::Tail;
};

template<Types ...types>
struct DecodeTypeF<StructBegin, types...>
{
    using DecodeHead = DecodeTypeF<types...>;
    using DecodeTail = GetTypeF<MakeStruct<typename DecodeHead::Tail>>;
    using Type = lib::typetraits::Concat<
        lib::typetraits::List<typename DecodeHead::Type>,
        typename DecodeTail::Type
    >;
    using Tail = typename DecodeTail::Tail;
};

template<Types ...types>
struct DecodeTypeF<StructBegin, StructEnd, types...>
{
    using Type = lib::typetraits::List<>;
    struct Tail
    {
        constexpr static inline std::array<Types, sizeof...(types)> type {
            types...
        };
    };
};

template<class T, class Indexes>
struct GetTypeF
{
    using Decode = GetTypeF<T, std::make_index_sequence<T::type.size()>>;
    using Type = typename Decode::Type;
    using Tail = typename Decode::Tail;
};

template<class T, std::size_t ...I>
struct GetTypeF<T, std::index_sequence<I...>>
{
    using Decode = DecodeTypeF<T::type[I]...>;
    using Type = typename Decode::Type;
    using Tail = typename Decode::Tail;
};

template<class T>
struct ConvertListToTupleF
{
    using Type = T;
};

template<class T>
using ConvertListToTuple = typename ConvertListToTupleF<T>::Type;

template<class ...Ts>
struct ConvertListToTupleF<lib::typetraits::List<Ts...>>
{
    using Type = std::tuple<ConvertListToTuple<Ts>...>;
};

template<class ...Ts>
struct ConvertListToTupleF<lib::typetraits::List<Ts...>*>
{
    using Type = std::tuple<ConvertListToTuple<Ts>...>*;
};

template<class T>
using GetType = ConvertListToTuple<typename GetTypeF<T>::Type>;

TEST(grammar, test)
{
    using namespace lib::interpreter;
    const std::array functions = {
        FunctionInfo{"operator: lhs + rhs", "u32", {param("u32", "lhs"), param("u32", "rhs")}},
        FunctionInfo{"u32::constructor", "void", {param("u32", "this"), param("u32", "value")}}
    };
    auto context = make_context(init_types, null_variables, functions, null_text);
    compile_function(
        context,
        fn("u32", "bar")(param("u32", "lhs"), param("u32", "rhs"))(
            //var("a") = fn("foo")(var("lhs"), 5_l),
            //var("b") = fn("foo")(var("lhs"), 5_l),
            //ret(var("a") + var("b") + 10_l)
            ret(var("lhs") + var("rhs"))
        )
    );
}
