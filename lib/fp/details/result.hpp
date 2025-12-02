#pragma once
#include <lib/fp/details/awaiter.hpp>
#include <lib/typetraits/list.hpp>
#include <lib/typetraits/tag.hpp>
#include <lib/raw.storage.hpp>
#include <lib/mutex.hpp>


namespace lib::fp::details {
    class IBaseResult
    {
    public:
        [[nodiscard]] virtual bool ready() const noexcept = 0;
        [[nodiscard]] virtual bool subscribe(IExecutor& executor, IAwaiter& awaiter) noexcept = 0;
        virtual ~IBaseResult() noexcept = default;
    };

    template <class T>
    class IResult: public IBaseResult
    {
    public:
        [[nodiscard]] virtual const T& get() const = 0;
        [[nodiscard]] virtual const T& wait(IExecutor& executor) = 0;
    };

    template <class T>
    class Result: public IResult<T>
    {
        enum class State {
            Stop, Running, Ready, Failed
        };

        State state = State::Stop;
        RawStorage<typetraits::List<T, std::exception_ptr>> result;

        mutable data_structures::DLList subscribers;
        mutable Mutex mutex;

    private:
        void emit(IExecutor* executor) const noexcept
        {
            std::lock_guard lock(mutex);
            for (auto& subscriber: subscribers.Range<IAwaiter>()) {
                subscriber.wakeup(executor);
            }
        }

        virtual void add_task(IExecutor&) noexcept {};

    public:
        ~Result() override
        {
            switch (state) {
                case State::Ready:
                    result.destroy(std::in_place_type<T>);
                    break;
                case State::Failed:
                    result.destroy(std::in_place_type<std::exception_ptr>);
                    break;
                default:
                    break;
            }
        }

        template <class U>
        void set(IExecutor* executor, U&& value) noexcept
        {
            result.emplace(std::in_place_type<T>, std::forward<U>(value));
            state = State::Ready;
            emit(executor);
        }

        void set(typetraits::TTag<std::exception_ptr>, IExecutor* executor, const std::exception_ptr& error) noexcept
        {
            result.emplace(std::in_place_type<std::exception_ptr>, error);
            state = State::Failed;
            emit(executor);
        }

        [[nodiscard]] bool ready() const noexcept final
        {
            std::lock_guard lock(mutex);
            return state == State::Ready || state == State::Failed;
        }

        [[nodiscard]] const T& get() const final
        {
            std::lock_guard lock(mutex);
            switch (state) {
                case State::Ready:  return *result.ptr(std::in_place_type<T>);
                case State::Failed: rethrow_exception(*result.ptr(std::in_place_type<std::exception_ptr>));
                default: throw std::runtime_error("value is not ready");
            }
        }

        [[nodiscard]] bool subscribe(IExecutor& executor, IAwaiter& awaiter) noexcept final
        {
            std::lock_guard lock(mutex);
            switch (state) {
                case State::Stop:
                    subscribers.PushBack(awaiter);
                    state = State::Running;
                    add_task(executor);
                    return false;
                case State::Running:
                    subscribers.PushBack(awaiter);
                    return false;
                case State::Failed:
                case State::Ready:
                    return true;
            }
            return false;
        }

        [[nodiscard]] const T& wait(IExecutor& executor) final
        {
            Awaiter awaiter;
            if (!subscribe(executor, awaiter)) {
                executor.run();
                awaiter.wait();
            }
            return get();
        }
    };
}
