#pragma once
#include <lib/fp/base.function.hpp>
#include <lib/fp/executor.hpp>
#include <lib/array.hpp>


namespace lib::fp {

    template <class T, Signature<T> Signature>
    class Fn<Signature, ImplValue<T>>
    {
    public: // public types
        using ITypes = typetraits::List<>;
        using IParam = void;
        using Result = T;
        using TFImpl = ImplValue<T>;

    private:
        T value;

    public: // object interface
        template <class ...TArgs>
        requires std::constructible_from<T, TArgs&&...>
        Fn(TArgs&& ...args) noexcept(std::is_nothrow_constructible_v<T, TArgs&&...>)
        : value(std::forward<TArgs>(args)...)
        {}

        Fn(T&& value)
            noexcept(std::is_nothrow_move_constructible_v<T>)
            requires (!std::is_array_v<T>)
        : value(std::move(value))
        {}

        Fn(const T& value)
            noexcept(std::is_nothrow_copy_constructible_v<T>)
            requires (!std::is_array_v<T>)
        : value(value)
        {}

        template <std::size_t ...I>
        Fn(const T& array, std::index_sequence<I...>)
            noexcept(std::is_nothrow_copy_constructible_v<T>)
            requires (std::is_array_v<T>)
        : value {array[I]...}
        {}

        Fn(const T& value)
            noexcept(std::is_nothrow_copy_constructible_v<T>)
            requires (std::is_array_v<T>)
        : Fn(value, std::make_index_sequence<std::extent_v<T>>{})
        {}

        Fn() = delete;
        Fn(const Fn& other) = default;
        Fn(Fn&& other) = default;
        Fn& operator=(const Fn& other) = default;
        Fn& operator=(Fn&& other) = default;

    public: // currying interface
        const Fn& operator () () const noexcept
        {
            return *this;
        }

    public: // runtime interface
        const auto& operator()([[maybe_unused]] IExecutor& executor) const noexcept
        {
            return value;
        }

        auto& operator * () const noexcept
        {
            return *this;
        }

        [[nodiscard]] bool ready() const noexcept
        {
            return true;
        }

        [[nodiscard]] const T& get() const noexcept
        {
            return value;
        }

        bool subscribe([[maybe_unused]] IExecutor& executor, [[maybe_unused]] IAwaiter& awaiter) noexcept
        {
            return true;
        }
    };

    template <class T>
    requires (!std::is_invocable_v<std::remove_reference_t<T>>)
    Fn(T&& value) -> Fn<signature<std::remove_reference_t<T>>, ImplValue<std::remove_reference_t<T>>>;

    template <class T, std::size_t N>
    Fn(RawArray<T, N> value) -> Fn<signature<T*>, ImplValue<T*>>;

    unittest {
        SimpleExecutor executor;

        Fn value = 42;
        check(value(executor) == 42);
        Fn string = "hello";
        check(string(executor) == std::string("hello"));
    }
}
