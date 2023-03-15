#pragma once
#include <utility>

namespace lib
{
    template<class ...Ts>
    struct tag_t {};

    template<auto ...Vs>
    struct tag_v {};

    template<class ...Ts>
    constexpr inline tag_t<Ts...> tag;

    template<class T, class Tag>
    class type_with_tag
    {
        T value;
    public:
        using tag = Tag;
        template<class ...TArgs>
        constexpr type_with_tag(TArgs&& ...args) noexcept(std::is_nothrow_constructible_v<T, TArgs...>)
        : value(std::forward<TArgs>(args)...)
        {}
        type_with_tag(const type_with_tag&) = default;
        type_with_tag(type_with_tag&&) = default;
        type_with_tag& operator=(const type_with_tag&) = default;
        type_with_tag& operator=(type_with_tag&&) = default;
        operator const T&() const& noexcept
        {
            return value;
        }
        operator T&() & noexcept
        {
            return value;
        }
        operator T&&() && noexcept
        {
            return std::move(value);
        }
    };
}