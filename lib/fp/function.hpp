#pragma once
#include <lib/test.hpp>
#include <lib/mutex.hpp>
#include <lib/data-structures/dl-list.hpp>
#include <lib/index.sequence.hpp>

#include <memory>
#include <vector>
#include <optional>
#include <coroutine>
#include <semaphore>


namespace lib::fp {

    class IWorker
    {
    public:
        class ITask
        {
        public:
            virtual void run(IWorker* worker) noexcept = 0;
        };

    public:
        virtual void add(ITask* task) noexcept = 0;
        virtual ITask* get() noexcept = 0;
    };

    class SimpleWorker: public IWorker
    {
        std::vector<ITask*> tasks;

        void add(ITask* task) noexcept final
        {
            tasks.push_back(task);
        }

        ITask* get() noexcept final
        {
            if (tasks.empty()) {
                return nullptr;
            }

            auto* task = tasks.back();
            tasks.pop_back();
            return task;
        }
    };

    class IAwaiter: public data_structures::DLListElement<>
    {
    public:
        virtual void wakeup() noexcept = 0;
    };

    template <class T>
    class Fn;

    template <class T>
    class Fn<T()>
    {
        class IResult
        {
        public:
            [[nodiscard]] virtual bool ready() const noexcept = 0;
            [[nodiscard]] virtual const T& get() const noexcept = 0;
            virtual void subscribe(IWorker& worker, IAwaiter& awaiter) = 0;
        };

        std::shared_ptr<IResult> result;

    private:
        explicit Fn(typetraits::TTag<IResult>, std::shared_ptr<IResult> result) noexcept
        : result(std::move(result))
        {}

    public:
        class Promise: public IWorker::ITask, public IAwaiter
        {
            using Handle = std::coroutine_handle<Promise>;

            class Result: public IResult
            {
                Promise* promise;
                std::optional<T> result;
                std::exception_ptr exception = nullptr;

                mutable data_structures::DLList subscribers;
                mutable Mutex mutex;

            private:
                void emit() const noexcept
                {
                    for (auto& subscriber: subscribers.Range<IAwaiter>()) {
                        subscriber.wakeup();
                    }
                }

            public:
                Result(Promise* promise) noexcept
                : promise(promise)
                {}

                template <class U>
                void set(U&& value) noexcept
                {
                    std::lock_guard lock(mutex);
                    result.emplace(std::forward<U>(value));
                    emit();
                }

                void set(typetraits::TTag<std::exception_ptr>, const std::exception_ptr& error) noexcept
                {
                    exception = error;
                    emit();
                }

                [[nodiscard]] bool ready() const noexcept final
                {
                    return exception != nullptr || result.has_value();
                }

                [[nodiscard]] const T& get() const noexcept final
                {
                    if (exception) {
                        rethrow_exception(exception);
                    }
                    return *result;
                }

                void subscribe(IWorker& worker, IAwaiter& awaiter) final
                {
                    std::unique_lock lock(mutex);

                    if (ready()) {
                        awaiter.wakeup();
                    } else {
                        subscribers.PushBack(awaiter);
                        lock.unlock();
                        worker.add(promise);
                    }
                }
            };

            std::shared_ptr<Result> result = std::make_shared<Result>(this);
            mutable IWorker* worker = nullptr;

        private:
            void run(IWorker* worker) noexcept final
            {
                this->worker = worker;
                Handle::from_promise(*this).resume();
            }

            void wakeup() noexcept final
            {
                worker->add(this);
            }

        public:
            [[nodiscard]] Fn<T> get_return_object() const noexcept
            {
                return Fn<T>(typetraits::tag_t<IResult>, result);
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
                result->set(typetraits::tag_t<std::exception_ptr>, std::current_exception());
            }

            template <class U>
            void return_value(U&& value)
            {
                result->set(std::forward<U>(value));
            }

            template <class U>
            void return_value(Fn<U()> value)
            {
                result->set(value());
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

            template <class U>
            [[nodiscard]] auto await_transform(Fn<U> function) noexcept
            {
                class Awaiter
                {
                    Fn<U> function;

                public:
                    Awaiter( Fn<U> function) noexcept
                    : function(std::move(function))
                    {
                    }

                public:
                    [[nodiscard]] bool await_ready() const noexcept
                    {
                        return function.ready();
                    }

                    constexpr void await_suspend(std::coroutine_handle<Promise> handle) noexcept
                    {
                        auto& promise = handle.promise();
                        auto* worker = promise.worker;
                        function.subscribe(*worker, promise);
                    }

                    [[nodiscard]] decltype(auto) await_resume() const noexcept
                    {
                        return function.get();
                    }
                };

                return Awaiter(std::move(function));
            }
        };

        using promise_type = Promise;

    public:
        Fn(const Fn& other) noexcept = default;
        Fn(Fn&& other) noexcept = default;
        Fn& operator=(const Fn& other) noexcept = default;
        Fn& operator=(Fn&& other) noexcept = default;

        template <class Function>
        requires (!std::is_same_v<std::remove_cvref_t<Function>, Fn>)
        Fn(Function&& function)
        {
            class Impl: public IResult, public IWorker::ITask
            {
                Function function;
                std::optional<T> value;

                mutable data_structures::DLList subscribers;
                mutable Mutex mutex;

            private:
                void emit() const noexcept
                {
                    for (auto& subscriber: subscribers.Range<IAwaiter>()) {
                        subscriber.wakeup();
                    }
                }

            public:
                Impl(Function&& function)
                : function(std::forward<Function>(function))
                {}

                [[nodiscard]] bool ready() const noexcept final
                {
                    return value.has_value();
                }

                [[nodiscard]] const T& get() const noexcept final
                {
                    return *value;
                }

                void run(IWorker*) noexcept final
                {
                    std::lock_guard lock(mutex);
                    value = function();
                    emit();
                }

                void subscribe(IWorker& worker, IAwaiter& awaiter) final
                {
                    std::unique_lock lock(mutex);

                    if (ready()) {
                        lock.unlock();
                        awaiter.wakeup();
                    } else {
                        subscribers.PushBack(awaiter);
                        lock.unlock();
                        worker.add(this);
                    }
                }
            };

            result = std::make_shared<Impl>(std::forward<Function>(function));
        }

        Fn(T value)
        {
            class Impl: public IResult
            {
                T value;

            public:
                Impl(T value)
                : value(std::move(value))
                {}

                [[nodiscard]] bool ready() const noexcept final
                {
                    return true;
                }

                [[nodiscard]] const T& get() const noexcept final
                {
                    return value;
                }

                void subscribe(IWorker&, IAwaiter& awaiter) final
                {
                    awaiter.wakeup();
                }
            };

            result = std::make_shared<Impl>(std::move(value));
        }

        const T& operator()(IWorker& worker) const
        {
            class Awaiter: public IAwaiter
            {
                mutable std::binary_semaphore sem {0};

            public:
                void wakeup() noexcept final
                {
                    sem.release();
                }

                void wait() noexcept
                {
                    sem.acquire();
                }
            };

            Awaiter awaiter;
            result->subscribe(worker, awaiter);
            while (auto* task = worker.get()) {
                task->run(&worker);
            }
            awaiter.wait();
            return result->get();
        }

        [[nodiscard]] bool ready() const noexcept
        {
            return result->ready();
        }

        [[nodiscard]] const T& get() const noexcept
        {
            return result->get();
        }
    
        void subscribe(IWorker& worker, IAwaiter& awaiter)
        {
            result->subscribe(worker, awaiter);
        }
    };

    template <class T>
    class Fn: public Fn<T()>
    {
    public:
        using Fn<T()>::Fn;
    };

    template <class T>
    Fn(T value) ->Fn<T>;

    unittest {
        SimpleWorker worker;

        Fn<int> function = [] { return 42; };
        check(function(worker) == 42);
        function = 13;
        check(function(worker) == 13);
    };

    namespace details {
        inline Fn<int> test_coro()
        {
            co_return 42;
        }

        unittest {
            SimpleWorker worker;

            const auto function = test_coro();
            check(function(worker) == 42);
        }

        inline Fn<int> test_coro(Fn<int> param)
        {
            co_return co_await param + co_await 1;
        }

        unittest {
            SimpleWorker worker;

            const auto function = test_coro(test_coro());
            check(function(worker) == 43);
        }
    }

    template <class T>
    struct FnWrapper
    {
        using Type = Fn<T>;
    };

    template <class T>
    struct FnWrapper<Fn<T>>
    {
        using Type = Fn<T>;
    };

    template <class T>
    using Wrapper = typename FnWrapper<T>::Type;


    template <class R, class ...TArgs>
    class Fn<R(TArgs...)>
    {
        class IFunction
        {
        public:
            [[nodiscard]] virtual Fn<R> run(Wrapper<TArgs>...) const = 0;
        };

        std::shared_ptr<IFunction> ifunction;

    public:
        Fn(const Fn& other) noexcept = default;
        Fn(Fn&& other) noexcept = default;
        Fn& operator=(const Fn& other) noexcept = default;
        Fn& operator=(Fn&& other) noexcept = default;

        template <class Function>
        requires (!std::is_same_v<std::remove_cvref_t<Function>, Fn>)
        Fn(Function&& function)
        {
            class Impl: public IFunction
            {
                Function function;

            public:
                Impl(Function&& function)
                : function(std::forward<Function>(function))
                {}

                [[nodiscard]] Fn<R> run(Wrapper<TArgs>... args) const final
                {
                    co_return co_await function(co_await args...);
                }
            };

            ifunction = std::make_shared<Impl>(std::forward<Function>(function));
        }

    private:
        template <std::size_t ...I, class ...IArgs>
        [[nodiscard]] auto currying(std::index_sequence<I...>, IArgs ...cargs) const
        {
            using Function = Fn<R(std::tuple_element_t<I, std::tuple<TArgs...>>...)>;
            Function function = [=, ifunction = this->ifunction] (std::tuple_element_t<I, std::tuple<TArgs...>>... iargs) -> Fn<R> {
                co_return co_await ifunction->run(cargs..., iargs...);
            };
            return function;
        }

    public:
        template <class ...IArgs>
        auto operator()(IArgs ...args) const
        {
            constexpr auto fparams = sizeof...(TArgs);
            constexpr auto iparams = sizeof...(IArgs);
            static_assert(iparams <= fparams);

            if constexpr (iparams == fparams) {
                return ifunction->run(args...);
            } else {
                return currying(indexes::range<fparams - 1U, fparams - iparams>(), args...);
            }
        }
    };

    unittest {
        SimpleWorker worker;

        Fn<int(int, int)> sum = [] (int lhs, int rhs) noexcept {
            return lhs + rhs;
        };
        check(sum(1, 2)(worker) == 3);
        check(sum(3)(2)(worker) == 5);
    }

    unittest {
        SimpleWorker worker;

        const auto sum_coro = [](Fn<int> lhs, Fn<int> rhs) -> Fn<int> {
            const auto l = co_await lhs;
            const auto r = co_await rhs;
            co_return l + r;
        };

        Fn<int(int, int)> sum = sum_coro;
        check(sum(1, 2)(worker) == 3);
        check(sum(3)(2)(worker) == 5);
    }

    unittest {
        SimpleWorker worker;

        Fn<std::string(std::string_view, const char*)> concat = [](std::string_view lhs, const char* rhs) -> std::string {
            std::string result(lhs.begin(), lhs.end());
            result.append(rhs);
            return result;
        };
        check(concat(std::string_view("hello"), "world")(worker) == "helloworld");
        check(concat(std::string_view("hello"))("world")(worker) == "helloworld");
    }
}
