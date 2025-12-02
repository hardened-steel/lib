#pragma once
#include <lib/fp/base.function.hpp>
#include <lib/fp/details/result.hpp>

#include <coroutine>
#include <memory>


namespace lib::fp::details {
    template <class T>
    class Promise: private IAwaiter, private IExecutor::ITask
    {
    public:
        using Type = T;
        using Handle = std::coroutine_handle<Promise>;
        class CoroResult: public Result<Type>
        {
            Promise* promise;

        public:
            CoroResult(Promise* promise) noexcept
            : promise(promise)
            {}

            void add_task(IExecutor& executor) noexcept final
            {
                executor.add(promise);
            }
        };

    private:
        std::shared_ptr<CoroResult> result = std::make_shared<CoroResult>(this);
        mutable IExecutor* executor = nullptr;

    private:
        void run(IExecutor* executor) noexcept final
        {
            this->executor = executor;
            Handle::from_promise(*this).resume();
        }

        void wakeup(IExecutor* executor) noexcept final
        {
            executor->add(this);
        }

    public:
        [[nodiscard]] Val<Type> get_return_object() const noexcept
        {
            return Val<Type>(typetraits::tag_t<CoroResult>, result);
        }

        auto initial_suspend() const noexcept // NOLINT
        {
            return std::suspend_always{};
        }

        auto final_suspend() const noexcept // NOLINT
        {
            return std::suspend_never{};
        }

        void unhandled_exception() noexcept
        {
            result->set(typetraits::tag_t<std::exception_ptr>, executor, std::current_exception());
        }

        template <class U>
        void return_value(U&& value)
        {
            result->set(executor, std::forward<U>(value));
        }

        template <class U>
        [[nodiscard]] auto await_transform(U&& value) const noexcept
        {
            struct Awaiter: std::suspend_never
            {
                U&& value;

                [[nodiscard]] U&& await_resume() const noexcept
                {
                    return std::forward<U>(value);
                }
            };
            return Awaiter{{}, std::forward<U>(value)};
        }

        template <class U, class Impl, Signature<U> Signature>
        [[nodiscard]] auto await_transform(Fn<Signature, Impl> function) noexcept(std::is_nothrow_move_constructible_v<Fn<Signature, Impl>>)
        {
            class Awaiter
            {
                using Function = Fn<Signature, Impl>;
                Function function;

            public:
                Awaiter(Function function) noexcept(std::is_nothrow_move_constructible_v<Function>)
                : function(std::move(function))
                {}

            public:
                [[nodiscard]] bool await_ready() const noexcept
                {
                    return function.ready();
                }

                [[nodiscard]] bool await_suspend(std::coroutine_handle<Promise> handle) noexcept
                {
                    auto& promise = handle.promise();
                    auto* executor = promise.executor;
                    return !function.subscribe(*executor, promise);
                }

                [[nodiscard]] decltype(auto) await_resume() const
                {
                    return function.get();
                }
            };

            return Awaiter(std::move(function));
        }
    };
}
