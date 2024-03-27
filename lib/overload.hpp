#pragma once
#include <array>
#include <tuple>
#include <utility>
#include <variant>

namespace lib {
    template<class... Ts>
    struct Overload: Ts...
    {
        using Ts::operator()...;
    };

    template<class... Ts>
    Overload(Ts...) -> Overload<Ts...>;

    namespace details {

        template<class T, class Tuple, std::size_t ...I>
        auto rswitch(T index, const Tuple& cases, std::index_sequence<I...>)
        {
            using Result = std::common_type_t<
                std::invoke_result_t<decltype(std::get<I>(cases))>...
            >;
            static const std::array<Result(*)(const Tuple& cases), sizeof...(I)> functions = {
                [](const Tuple& cases) { return std::get<I>(cases)(); } ...
            };
            return functions[index](cases);
        }

        template<class ...Types, class Tuple, std::size_t ...I>
        auto rswitch(const std::variant<Types...>& value, const Tuple& cases, std::index_sequence<I...>)
        {
            using Result = std::common_type_t<
                std::invoke_result_t<decltype(std::get<I>(cases)), decltype(*std::get_if<I>(&value))>...
            >;
            constexpr bool isNoExcept = (std::is_nothrow_invocable_v<decltype(std::get<I>(cases)), decltype(*std::get_if<I>(&value))> && ...);
            static const std::array<Result(*)(const std::variant<Types...>& value, const Tuple& cases) noexcept(isNoExcept), sizeof...(I)> functions = {
                [](const std::variant<Types...>& value, const Tuple& cases) noexcept(isNoExcept) { return std::get<I>(cases)(*std::get_if<I>(&value)); } ...
            };
            return functions[value.index()](value, cases);
        }

        template<class ...Types, class Tuple, std::size_t ...I>
        auto rswitch(std::variant<Types...>&& value, const Tuple& cases, std::index_sequence<I...>)
        {
            using Result = std::common_type_t<
                std::invoke_result_t<decltype(std::get<I>(cases)), decltype(std::move(*std::get_if<I>(&value)))>...
            >;
            constexpr bool isNoExcept = (std::is_nothrow_invocable_v<decltype(std::get<I>(cases)), decltype(*std::get_if<I>(&value))> && ...);
            static const std::array<Result(*)(std::variant<Types...>&& value, const Tuple& cases) noexcept(isNoExcept), sizeof...(I)> functions = {
                [](std::variant<Types...>&& value, const Tuple& cases) noexcept(isNoExcept) { return std::get<I>(cases)(std::move(*std::get_if<I>(&value))); } ...
            };
            return functions[value.index()](std::move(value), cases);
        }

    }

    template<class T, class ...TCases>
    auto rswitch(T index, TCases&& ...cases)
    {
        return details::rswitch(index, std::make_tuple(std::forward<TCases>(cases)...), std::make_index_sequence<sizeof...(TCases)>{});
    }

    template<class ...Types, class ...TCases>
    auto rswitch(const std::variant<Types...>& value, TCases&& ...cases)
    {
        return details::rswitch(value, std::make_tuple(std::forward<TCases>(cases)...), std::make_index_sequence<sizeof...(TCases)>{});
    }

    template<class ...Types, class ...TCases>
    auto rswitch(std::variant<Types...>&& value, TCases&& ...cases)
    {
        return details::rswitch(std::move(value), std::make_tuple(std::forward<TCases>(cases)...), std::make_index_sequence<sizeof...(TCases)>{});
    }
}
