#pragma once
#include <lib/fp/base.function.hpp>
#include <lib/fp/details/result.hpp>
#include <lib/fp/details/function.ptr.hpp>

#include <memory>


namespace lib::fp::details {
    template <class R, class ...TArgs>
    auto* CreateFunctionPtr(const R(*function)(TArgs...)) noexcept
    {
        return function;
    }

    template <class T>
    auto CreateFunctionPtr(T&& function)
    {
        return std::make_shared<std::remove_cvref_t<T>>(std::forward<T>(function));
    }

    template <auto Signature, class Impl>
    auto& CreateFunctionPtr(const Fn<Signature, Impl>& function) noexcept
    {
        return function;
    }

    template <class T, bool Cast>
    decltype(auto) unwrap(T&& arg, typetraits::VTag<Cast>)
    {
        if constexpr (Cast) {
            return std::forward<T>(arg);
        } else {
            return std::forward<T>(arg).get();
        }
    }

    template <class T, std::size_t A, std::size_t B>
    decltype(auto) check_cast(T&& arg, typetraits::VTag<A, B>)
    {
        return unwrap(std::forward<T>(arg), typetraits::tag_v<A == B>);
    }

    template <class Function, class TArgs, std::size_t I, class Indexes>
    struct CheckCast;

    template <class Function, class TArgs, std::size_t I, std::size_t ...Is>
    struct CheckCast<Function, TArgs, I, std::index_sequence<Is...>>
    {
        constexpr static inline bool value = std::invocable<Function, decltype(check_cast(std::get<Is>(std::declval<TArgs>()), typetraits::tag_v<I, Is>))...>; 
    };

    template <class Function, std::size_t I, class TArgs>
    concept could_call_without_cast = CheckCast<Function, TArgs, I, std::make_index_sequence<std::tuple_size_v<TArgs>>>::value;

    template <class Function, class ER, class FR, class TArgs>
    class FunctionResultImpl;

    template <class Function, class R, class ...TArgs>
    class FunctionResultImpl<Function, R, R, typetraits::List<TArgs...>>
        : public Result<R>
        , private IExecutor::ITask
        , private IAwaiter
    {
        FunctionPtr<Function> function;
        std::tuple<TArgs...> args;
        mutable std::size_t args_ready = 0;

    public:
        FunctionResultImpl(FunctionPtr<Function>&& function, std::tuple<TArgs...> args)
            noexcept(std::is_nothrow_move_constructible_v<Function>)
        : function(std::move(function)), args(args)
        {}

        FunctionResultImpl(const FunctionPtr<Function>& function, std::tuple<TArgs...> args)
            noexcept(std::is_nothrow_copy_constructible_v<Function>)
        : function(function), args(args)
        {}

    private:
        void add_task(IExecutor& executor) noexcept final
        {
            executor.add(this);
        }

        template <std::size_t ...I>
        void apply(IExecutor* executor, std::index_sequence<I...>) noexcept
        try {
            Result<R>::set(executor, (*function)(unwrap(std::get<I>(args), typetraits::tag_v<could_call_without_cast<Function, I, std::tuple<TArgs...>>>)...));
        } catch (...) {
            Result<R>::set(typetraits::tag_t<std::exception_ptr>, executor, std::current_exception());
        }

        void wakeup(IExecutor* executor) noexcept final
        {
            if (++args_ready == sizeof...(TArgs)) {
                executor->add(this);
            }
        }

        template <std::size_t I>
        std::size_t subscribe_arg(IExecutor* executor) noexcept
        {
            if constexpr (could_call_without_cast<Function, I, std::tuple<TArgs...>>) {
                return 1U;
            } else {
                if (std::get<I>(args).subscribe(*executor, *this)) {
                    return 1U;
                } else {
                    return 0U;
                }
            }
        }

        template <std::size_t ...I>
        void run(IExecutor* executor, std::index_sequence<I...>) noexcept
        {
            args_ready = (subscribe_arg<I>(executor) + ... + 0);
            if (args_ready == sizeof...(TArgs)) {
                executor->add(this);
            }
        }

        void run(IExecutor* executor) noexcept final
        {
            if (args_ready == 0) {
                return run(executor, std::make_index_sequence<sizeof...(TArgs)>{});
            }
            if (args_ready != 0) {
                return apply(executor, std::make_index_sequence<sizeof...(TArgs)>{});
            }
        }
    };

    template <class Function, class R>
    class FunctionResultImpl<Function, R, R, typetraits::List<>>: public details::IResult<R>
    {
        std::optional<R> value;
        std::exception_ptr exception;

    public:
        FunctionResultImpl(const FunctionPtr<Function>& function, const std::tuple<>&) noexcept
        {
            try {
                value.emplace((*function)());
            } catch (...) {
                exception = std::current_exception();
            }
        }

        [[nodiscard]] bool ready() const noexcept final
        {
            return true;
        }

        [[nodiscard]] const R& get() const final
        {
            if (exception) {
                std::rethrow_exception(exception);
            }
            return *value;
        }

        [[nodiscard]] bool subscribe(IExecutor&, IAwaiter&) noexcept final
        {
            return true;
        }

        [[nodiscard]] const R& wait(IExecutor&) final
        {
            return get();
        }
    };

    template <class Function, class R, class Impl, class ...TArgs>
    class FunctionResultImpl<Function, R, Fn<signature<R>, Impl>, typetraits::List<TArgs...>>
        : public IResult<R>
        , private IExecutor::ITask
        , private IAwaiter
    {
        FunctionPtr<Function> function;
        std::tuple<TArgs...> args;
        mutable std::size_t args_ready = 0;

        enum class State {
            Stop, Prepare, Running, Ready
        };
        State state = State::Stop;
        RawStorage<Fn<signature<R>, Impl>> result;
        std::exception_ptr exception = nullptr;

        mutable data_structures::DLList subscribers;
        mutable Mutex mutex;

    private:
        void emit(IExecutor* executor) const noexcept
        {
            for (auto& subscriber: subscribers.Range<IAwaiter>()) {
                subscriber.wakeup(executor);
            }
        }

    public:
        FunctionResultImpl(const FunctionPtr<Function>& function, const std::tuple<TArgs...>& args)
            noexcept(std::is_nothrow_copy_constructible_v<Function>)
        : function(function), args(args)
        {}

        ~FunctionResultImpl() override
        {
            if (exception == nullptr && (state == State::Ready || state == State::Running)) {
                result.destroy();
            }
        }

        [[nodiscard]] bool ready() const noexcept final
        {
            std::lock_guard lock(mutex);
            return state == State::Ready;
        }

        [[nodiscard]] const R& get() const final
        {
            if (exception) {
                rethrow_exception(exception);
            }
            return result.ptr()->get();
        }

        bool subscribe(IExecutor& executor, IAwaiter& awaiter) noexcept final
        {
            std::lock_guard lock(mutex);
            switch (state) {
                case State::Stop:
                    subscribers.PushBack(awaiter);
                    state = State::Prepare;
                    executor.add(this);
                    return false;
                case State::Prepare:
                case State::Running:
                    subscribers.PushBack(awaiter);
                    return false;
                case State::Ready:
                    return true;
            }
            return false;
        }

        const R& wait(IExecutor& executor) final
        {
            Awaiter awaiter;
            if (!subscribe(executor, awaiter)) {
                executor.run();
                awaiter.wait();
            }
            return get();
        }

    private:
        template <std::size_t ...I>
        void apply(IExecutor* executor, std::index_sequence<I...>) noexcept
        try {
            auto& fn = *result.emplace((*function)(unwrap(std::get<I>(args), typetraits::tag_v<could_call_without_cast<Function, I, std::tuple<TArgs...>>>)...));
            if (fn.subscribe(*executor, *this)) {
                state = State::Ready;
                emit(executor);
            } else {
                state = State::Running;
            }
        } catch (...) {
            state = State::Ready;
            exception = std::current_exception();
            emit(executor);
        }

        void wakeup(IExecutor* executor) noexcept final
        {
            std::lock_guard lock(mutex);
            if (state == State::Prepare) {
                if (++args_ready == sizeof...(TArgs)) {
                    executor->add(this);
                }
            }
            if (state == State::Running) {
                state = State::Ready;
                emit(executor);
            }
        }

        template <std::size_t I>
        std::size_t subscribe_arg(IExecutor* executor) noexcept
        {
            if constexpr (could_call_without_cast<Function, I, std::tuple<TArgs...>>) {
                return 1U;
            } else {
                if (std::get<I>(args).subscribe(*executor, *this)) {
                    return 1U;
                } else {
                    return 0U;
                }
            }
        }

        template <std::size_t ...I>
        void run(IExecutor* executor, std::index_sequence<I...>) noexcept
        {
            args_ready = (subscribe_arg<I>(executor) + ... + 0);
            if (args_ready == sizeof...(TArgs)) {
                executor->add(this);
            }
        }

        void run(IExecutor* executor) noexcept final
        {
            if (args_ready != 0) {
                return apply(executor, std::make_index_sequence<sizeof...(TArgs)>{});
            }
            if (args_ready == 0) {
                return run(executor, std::make_index_sequence<sizeof...(TArgs)>{});
            }
        }
    };

    template <class Function, class TArgs, class Indexes = std::make_index_sequence<std::tuple_size_v<TArgs>>>
    struct InvokeResultF;

    template <class Function, class ...TArgs>
    using InvokeResult = typename InvokeResultF<Function, std::tuple<TArgs...>>::Result;

    template <class Function, class ...TArgs, std::size_t ...I>
    struct InvokeResultF<Function, std::tuple<TArgs...>, std::index_sequence<I...>>
    {
        using Result = decltype(
            std::declval<Function>()(unwrap(std::declval<TArgs>(), typetraits::tag_v<could_call_without_cast<Function, I, std::tuple<TArgs...>>>)...)
        );
    };

    template <class Function, class R, class ...TArgs>
    using FunctionResult = FunctionResultImpl<Function, std::decay_t<R>, InvokeResult<Function, TArgs...>, typetraits::List<TArgs...>>;
}
