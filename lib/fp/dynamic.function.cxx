#pragma once
#include <lib/fp/function.hpp>


namespace lib::fp {
    namespace details {
        struct DynamicFunction {};
    }

    template <class T>
    class Fn<T, details::DynamicFunction>
    {
        class IResult
        {
        public:
            [[nodiscard]] virtual bool ready() const noexcept = 0;
            [[nodiscard]] virtual T get() const = 0;
            virtual void run() = 0;
        };

        std::shared_ptr<IResult> result;

    public:
        Fn(const Fn& other) noexcept = default;
        Fn(Fn&& other) noexcept = default;
        Fn& operator=(const Fn& other) noexcept = default;
        Fn& operator=(Fn&& other) noexcept = default;

        template <class Function>
        requires (!std::is_same_v<std::remove_cvref_t<Function>, Fn>)
        Fn(Function&& function)
        {
            class Impl: public IResult
            {
                Function function;
                std::optional<T> value;

            public:
                Impl(Function&& function)
                : function(std::forward<Function>(function))
                {}

                [[nodiscard]] bool ready() const noexcept final
                {
                    return value.has_value();
                }

                [[nodiscard]] T get() const final
                {
                    return *value;
                }

                void run() final
                {
                    value.emplace(function());
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

                [[nodiscard]] T get() const final
                {
                    return value;
                }

                void run() final
                {
                }
            };

            result = std::make_shared<Impl>(std::move(value));
        }

        T operator()() const
        {
            if (!result->ready()) {
                result->run();
            }
            return result->get();
        }
    };
}
