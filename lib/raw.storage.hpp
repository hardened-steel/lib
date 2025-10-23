#pragma once
#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <lib/typetraits/list.hpp>

namespace lib {
    template <class T>
    class RawStorage
    {
    private:
        alignas(T) std::byte buffer[sizeof(T)]; // NOLINT
    public:
        T* ptr() noexcept
        {
            return std::launder(reinterpret_cast<T*>(buffer));
        }

        const T* ptr() const noexcept
        {
            return std::launder(reinterpret_cast<const T*>(buffer));
        }

        template <class ...TArgs>
        T* emplace(TArgs&& ...args)
        noexcept(std::is_nothrow_constructible_v<T, TArgs...>)
        {
            return std::construct_at(reinterpret_cast<T*>(buffer), std::forward<TArgs>(args)...);
        }

        void destroy() noexcept
        {
            std::destroy_at(ptr());
        }
    };

    template <class ...Ts>
    class RawStorage<typetraits::List<Ts...>>
    {
    private:
        alignas(Ts...) std::byte buffer[std::max({sizeof(Ts)...})]; // NOLINT
    public:
        template <class T>
        T* ptr(std::in_place_type_t<T>) noexcept
        {
            return reinterpret_cast<T*>(buffer);
        }

        template <class T>
        const T* ptr(std::in_place_type_t<T>) const noexcept
        {
            return reinterpret_cast<const T*>(buffer);
        }

        template <class T, class ...TArgs>
        T* emplace(std::in_place_type_t<T>, TArgs&& ...args)
        noexcept(std::is_nothrow_constructible_v<T, TArgs...>)
        {
            return std::construct_at(ptr(std::in_place_type<T>), std::forward<TArgs>(args)...);
        }

        template <class T>
        void destroy(std::in_place_type_t<T>) noexcept
        {
            std::destroy_at(ptr(std::in_place_type<T>));
        }
    };
}
