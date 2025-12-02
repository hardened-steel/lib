#pragma once
#include <lib/platform.hpp>
#include <lib/static.string.hpp>
#include <lib/typetraits/tag.hpp>

#include <source_location>
#include <ranges>
#include <stdexcept>


#if LIB_HOST_COMPILER == CLANG
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wundefined-inline"
#endif

#if LIB_HOST_COMPILER == GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif

namespace lib::typetraits {
    namespace details {

        struct Location
        {
            std::size_t line;
            std::size_t column;
            char file[256] = {}; // NOLINT
            char function[256] = {}; // NOLINT

            friend constexpr auto operator <=> (const Location& lhs, const Location& rhs) noexcept = default;
        };

        constexpr Location next (std::source_location location) noexcept
        {
            Location result {
                .line   = location.line(),
                .column = location.column()
            };
            std::string_view file = location.file_name();
            for (auto i: std::views::iota(std::size_t(0), std::min(sizeof(Location::file) - 1, file.size()))) {
                result.file[i] = file[i];
            }

            std::string_view function = location.function_name();
            for (auto i: std::views::iota(std::size_t(0), std::min(sizeof(Location::function) - 1, function.size()))) {
                result.function[i] = function[i];
            }
            return result;
        }

        template <StaticString Name, std::size_t Version>
        struct Var
        {
            friend constexpr auto& current_value(Var<Name, Version>) noexcept;
        };

        template <StaticString Name, std::size_t Version, auto Value>
        struct Set
        {
            constexpr static inline auto value = Value;

            friend constexpr auto& current_value(Var<Name, Version>) noexcept
            {
                return value;
            }
        };

        template <class, class>
        consteval bool is_var_exist_impl (...) noexcept { return false; }
        
        template <class, class T, auto = current_value(T{})>
        consteval bool is_var_exist_impl (T**) noexcept { return true; }

        template <StaticString Name, std::size_t I, class Tag>
        constexpr bool is_var_exist = is_var_exist_impl<Tag, Var<Name, I>>(nullptr);

        template <StaticString Name, auto Value, class Tag, std::size_t I>
        constexpr auto& set_impl() noexcept
        {
            if constexpr (is_var_exist<Name, I, Tag>) {
                return set_impl<Name, Value, Tag, I + 1>();
            } else {
                return Set<Name, I, Value>::value;
            }
        }

        template <StaticString Name, class Tag, std::size_t I>
        constexpr auto& get_impl()
        {
            if constexpr (is_var_exist<Name, I, Tag>) {
                return get_impl<Name, Tag, I + 1>();
            } else {
                if constexpr (I == 0) {
                    throw std::runtime_error("variable didn't assign");
                } else {
                    return current_value(Var<Name, I - 1>{});
                }
            }
        }
    }

    using Location = details::Location;

    constexpr auto next(std::source_location location = std::source_location::current()) noexcept
    {
        return details::next(location);
    }

    template <StaticString Name, auto Value, class Tag, std::size_t I = 0>
    constexpr auto& set = details::set_impl<Name, Value, Tag, I>();

    template <StaticString Name, class Tag, std::size_t I = 0>
    constexpr auto& get = details::get_impl<Name, Tag, I>();

    // examples
    static_assert(set<"test-value", 42, VTag<next()>> == 42);
    static_assert(get<"test-value", VTag<next()>> == 42);
    static_assert(get<"test-value", VTag<next()>> == 42);
    static_assert(set<"test-value", 13, VTag<next()>> == 13);
    static_assert(get<"test-value", VTag<next()>> == 13);
    static_assert(get<"test-value", VTag<next()>> == 13);
    static_assert(set<"test-value", get<"test-value", VTag<next()>> - 1, VTag<next()>> == 12);
}

#if LIB_HOST_COMPILER == CLANG || LIB_HOST_COMPILER == GCC
#   pragma GCC diagnostic pop
#endif
