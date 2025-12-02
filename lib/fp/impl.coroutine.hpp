#pragma once
#include <lib/fp/base.function.hpp>
#include <lib/fp/impl.function.hpp>
#include <lib/fp/details/promise.hpp>


namespace lib::fp {
    template <class T, Signature<T> Signature>
    class Fn<Signature, Dynamic>
    {
        friend details::Promise<T>;

    public: // public types
        using ITypes = typetraits::List<>;
        using IParam = void;
        using Result = T;
        using TFImpl = Dynamic;

        using promise_type = details::Promise<T>;   

    private:
        std::shared_ptr<details::IResult<T>> result;  

    private:
        explicit Fn(typetraits::TTag<typename details::Promise<T>::CoroResult>, std::shared_ptr<details::IResult<T>> result) noexcept
        : result(std::move(result))
        {}

    public: // object interface
        Fn(const Fn& other) noexcept = default;
        Fn(Fn&& other) noexcept = default;
        Fn& operator=(const Fn& other) noexcept = default;
        Fn& operator=(Fn&& other) noexcept = default;

        template <class Function>
        requires (!std::is_same_v<std::remove_cvref_t<Function>, Fn> && std::is_invocable_r_v<T, std::remove_cvref_t<Function>>)
        Fn(Function&& function)
        : result(std::make_shared<typename Fn<signature<T>, ImplFCall<std::remove_cvref_t<Function>>>::FunctionResult>(std::forward<Function>(function)))
        {}

        template <class Function, class ...TArgs>
        Fn(const Fn<signature<T>, ImplFCall<Function, TArgs...>>& function) noexcept
        : result(function.result)
        {}

        Fn(T value)
        {
            class Impl: public details::Result<T>
            {
            public:
                Impl(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
                {
                    details::Result<T>::set(nullptr, std::move(value));
                }
            };

            result = std::make_shared<Impl>(std::move(value));
        }

    public: // currying interface
        const Fn& operator () () const noexcept
        {
            return *this;
        }

    public: // runtime interface
        const auto& operator()(IExecutor& executor) const
        {
            return result->wait(executor);
        }

        auto& operator * () const noexcept
        {
            return *this;
        }

        [[nodiscard]] bool ready() const noexcept
        {
            return result->ready();
        }

        [[nodiscard]] const T& get() const
        {
            return result->get();
        }

        bool subscribe(IExecutor& executor, IAwaiter& awaiter) noexcept
        {
            return result->subscribe(executor, awaiter);
        }
    };

    namespace details {
        inline Val<int> test_coro()
        {
            co_return 42;
        }

        unittest {
            SimpleExecutor executor;

            const auto function = test_coro();
            check(function(executor) == 42);
        }

        inline Val<int> test_coro(Val<int> param)
        {
            co_return co_await param + co_await 1;
        }

        unittest {
            SimpleExecutor executor;

            const auto function = test_coro(test_coro());
            check(function(executor) == 43);
        }
    }
}

/*namespace std {
    template <class R, auto ...Signs, class ...Implementations>
    struct std::coroutine_traits<R, lib::fp::Fn<Signs, Implementations>...>
    {
        using promise_type = lib::fp::details::Promise<R>;
    };

    template <class R, class T, auto ...Signs, class ...Implementations>
    struct std::coroutine_traits<R, T&, lib::fp::Fn<Signs, Implementations>...>
    {
        using promise_type = lib::fp::details::Promise<R>;
    };

    template <class R, class T, auto ...Signs, class ...Implementations>
    struct std::coroutine_traits<R, const T&, lib::fp::Fn<Signs, Implementations>...>
    {
        using promise_type = lib::fp::details::Promise<R>;
    };

    template <auto Sign, auto ...Signs, class ...Implementations>
    struct std::coroutine_traits<lib::fp::Fn<Sign, lib::fp::Dynamic>, lib::fp::Fn<Signs, Implementations>...>
    {
        using promise_type = typename lib::fp::Fn<Sign, lib::fp::Dynamic>::promise_type;
    };

    template <auto Sign, class T, auto ...Signs, class ...Implementations>
    struct std::coroutine_traits<lib::fp::Fn<Sign, lib::fp::Dynamic>, T&, lib::fp::Fn<Signs, Implementations>...>
    {
        using promise_type = typename lib::fp::Fn<Sign, lib::fp::Dynamic>::promise_type;
    };

    template <auto Sign, class T, auto ...Signs, class ...Implementations>
    struct std::coroutine_traits<lib::fp::Fn<Sign, lib::fp::Dynamic>, const T&, lib::fp::Fn<Signs, Implementations>...>
    {
        using promise_type = typename lib::fp::Fn<Sign, lib::fp::Dynamic>::promise_type;
    };
}*/
