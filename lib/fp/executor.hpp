#pragma once
#include <lib/data-structures/dl-list.hpp>
#include <vector>


namespace lib::fp {

    class IExecutor
    {
    public:
        class ITask
        {
        public:
            virtual void run(IExecutor* executor) noexcept = 0;
        };

    public:
        virtual void add(ITask* task) noexcept = 0;
        virtual void run() noexcept = 0;
        virtual ITask* get() noexcept = 0;
    };

    class IAwaiter: public data_structures::DLListElement<>
    {
    public:
        virtual void wakeup(IExecutor* executor) noexcept = 0;
    };

    class SimpleExecutor: public IExecutor
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

        void run() noexcept final
        {
            while (auto* task = get()) {
                task->run(this);
            }
        }
    };
}
