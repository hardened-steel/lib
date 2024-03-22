#pragma once
#include <lib/static.string.hpp>

namespace lib {
    template<class Lhs, class Rhs>
    class LazyString
    {
        Lhs lhs;
        Rhs rhs;
    public:
        template<class TLhs, class TRhs>
        constexpr LazyString(TLhs&& lhs, TRhs&& rhs) noexcept
        : lhs(std::forward<TLhs>(lhs))
        , rhs(std::forward<TRhs>(rhs))
        {}

        constexpr LazyString(const LazyString& other) = default;
        constexpr LazyString(LazyString&& other) = default;
        constexpr LazyString& operator=(const LazyString& other) = default;
        constexpr LazyString& operator=(LazyString&& other) = default;

        constexpr auto size() const noexcept
        {
            return std::size(lhs) + std::size(rhs);
        }
        constexpr const char& operator[](std::size_t index) const noexcept
        {
            if (index < std::size(lhs)) {
                return lhs[index];
            }
            return rhs[index - std::size(lhs)];
        }
        constexpr char& operator[](std::size_t index) noexcept
        {
            if (index < std::size(lhs)) {
                return lhs[index];
            }
            return rhs[index];
        }
    public:
        class Iterator
        {
            LazyString* string;
            std::size_t index;
        public:
            using iterator_category = std::random_access_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = char;
            using pointer           = char*;
            using reference         = char&;
        public:
            constexpr explicit Iterator(LazyString* string, std::size_t index = 0) noexcept
            : string(string), index(index)
            {}
            constexpr Iterator(const Iterator& other) = default;
            constexpr Iterator& operator=(const Iterator& other) = default;
        public:
            constexpr auto& operator*() const noexcept
            {
                return (*string)[index];
            }
            constexpr Iterator& operator++()
            {
                index++;
                return *this;
            }
            constexpr Iterator& operator+(std::size_t seek)
            {
                index += seek;
                return *this;
            }
            constexpr Iterator& operator-(std::size_t seek)
            {
                index -= seek;
                return *this;
            }
            friend constexpr bool operator==(const Iterator& lhs, const Iterator& rhs) noexcept
            {
                return (lhs.string == rhs.string) && (lhs.index == rhs.index);
            }
            friend constexpr bool operator!=(const Iterator& lhs, const Iterator& rhs) noexcept
            {
                return !(lhs == rhs);
            }
        };
        constexpr auto begin() noexcept
        {
            return Iterator(this);
        }
        constexpr auto end() noexcept
        {
            return Iterator(this, size());
        }
    public:
        class ConstIterator
        {
            const LazyString* string;
            std::size_t index;
        public:
            using iterator_category = std::random_access_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = char;
            using pointer           = const char*;
            using reference         = const char&;
        public:
            constexpr explicit ConstIterator(const LazyString* string, std::size_t index = 0) noexcept
            : string(string), index(index)
            {}
            constexpr ConstIterator(const ConstIterator& other) = default;
            constexpr ConstIterator& operator=(const ConstIterator& other) = default;
        public:
            constexpr const auto& operator*() const noexcept
            {
                return (*string)[index];
            }
            constexpr ConstIterator& operator++()
            {
                index++;
                return *this;
            }
            constexpr ConstIterator& operator+(std::size_t seek)
            {
                index += seek;
                return *this;
            }
            constexpr ConstIterator& operator-(std::size_t seek)
            {
                index -= seek;
                return *this;
            }
            friend constexpr bool operator==(const ConstIterator& lhs, const ConstIterator& rhs) noexcept
            {
                return (lhs.string == rhs.string) && (lhs.index == rhs.index);
            }
            friend constexpr bool operator!=(const ConstIterator& lhs, const ConstIterator& rhs) noexcept
            {
                return !(lhs == rhs);
            }
            friend constexpr difference_type operator-(const ConstIterator& lhs, const ConstIterator& rhs) noexcept
            {
                return lhs.index - rhs.index;
            }
        };
        constexpr auto begin() const noexcept
        {
            return ConstIterator(this);
        }
        constexpr auto end() const noexcept
        {
            return ConstIterator(this, size());
        }
    public:
        friend std::ostream& operator<<(std::ostream& stream, const LazyString& string)
        {
            return stream << string.lhs << string.rhs;
        }
    };

    template<class Lhs, class Rhs>
    LazyString(Lhs&& lhs, Rhs&& rhs) -> LazyString<std::remove_cv_t<std::remove_reference_t<Lhs>>, std::remove_cv_t<std::remove_reference_t<Rhs>>>;

    template<class T>
    struct IsStringF
    {
        constexpr static inline bool value = std::is_convertible_v<T, std::string_view>;
    };

    template<class T>
    constexpr inline bool is_string = IsStringF<T>::value;

    template<std::size_t N>
    struct IsStringF<StaticString<N>>
    {
        constexpr static inline bool value = true;
    };

    template<class Lhs, class Rhs>
    struct IsStringF<LazyString<Lhs, Rhs>>
    {
        constexpr static inline bool value = true;
    };

    template<
        class String,
        typename = lib::Require<
            is_string<
                std::remove_cv_t<std::remove_reference_t<String>>
            >
        >
    >
    constexpr auto&& convert_string(String&& string) noexcept
    {
        return std::forward<String>(string);
    }

    template<std::size_t N>
    constexpr auto convert_string(const char (&string)[N]) noexcept
    {
        return std::string_view(string);
    }

    template<
        class Lhs, class Rhs,
        typename = lib::Require<
            is_string<std::remove_cv_t<std::remove_reference_t<Lhs>>>,
            is_string<std::remove_cv_t<std::remove_reference_t<Rhs>>>
        >
    >
    constexpr auto operator + (Lhs&& lhs, Rhs&& rhs) noexcept
    {
        return LazyString(convert_string(std::forward<Lhs>(lhs)), convert_string(std::forward<Rhs>(rhs)));
    }

    template<
        class Lhs, class Rhs,
        typename = lib::Require<is_string<Lhs>, is_string<Rhs>>
    >
    constexpr auto compare_strings(const Lhs& lhs, const Rhs& rhs) noexcept
    {
        if (std::size(lhs) == std::size(rhs)) {
            for (std::size_t i = 0; i < std::size(lhs); ++i) {
                if (lhs[i] != rhs[i]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    template<
        class Lhs, class Rhs,
        typename = lib::Require<is_string<Lhs>, is_string<Rhs>>
    >
    constexpr auto operator == (const Lhs& lhs, const Rhs& rhs) noexcept
    {
        return compare_strings(convert_string(lhs), convert_string(rhs));
    }
}
