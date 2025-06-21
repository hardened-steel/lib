#pragma once
#include <atomic>
#include <lib/raw.storage.hpp>
#include <lib/typename.hpp>

namespace lib::lockfree {

    template <class T>
    class MPMCQueue
    {
        alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> enqueue_pos;
        alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> dequeue_pos;
    public:
        struct Cell
        {
            std::atomic<size_t> sequence;
            RawStorage<T> storage;
        };

        constexpr static std::size_t StorageSize(std::size_t n) noexcept
        {
            std::size_t count = 0;
            // First n in the below condition
            // is for the case where n is 0
            if ((n > 0) && !((n & (n - 1)) > 0)) {
                return std::max(n, 2);
            }

            while( n != 0) {
                n >>= 1;
                count += 1;
            }
            return std::max(1 << count, 2);
        }
    private:
        Span<Cell> buffer;
    public:
        constexpr MPMCQueue(Span<Cell> buffer) noexcept
        : buffer(buffer)
        {
            assert((buffer.size() >= 2) && ((buffer.size() & (buffer.size() - 1)) == 0));

            for (size_t i = 0; i != buffer.size(); i += 1) {
                buffer[i].sequence.store(i, std::memory_order_relaxed);
            }

            enqueue_pos.store(0, std::memory_order_relaxed);
            dequeue_pos.store(0, std::memory_order_relaxed);
        }

        constexpr ~MPMCQueue() noexcept
        {
            const auto buffer_mask = buffer.size() - 1;
            Cell *cell = nullptr;

            size_t pos = dequeue_pos.load(std::memory_order_relaxed);
            for (;;) {
                cell = &buffer[pos & buffer_mask];
                size_t seq = cell->sequence_.load(std::memory_order_acquire);
                intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

                if (diff == 0)
                {
                    cell->storage.destroy();
                    cell->sequence_.store(pos + buffer_mask + 1, std::memory_order_release);
                    pos += 1;
                } else {
                    return;
                }
            }
        }

        bool enqueue(T& data)
        {
            const auto buffer_mask = buffer.size() - 1;
            Cell *cell = nullptr;

            size_t pos = enqueue_pos.load(std::memory_order_relaxed);
            for (;;) {
                cell = &buffer[pos & buffer_mask];
                size_t seq = cell->sequence_.load(std::memory_order_acquire);

                intptr_t diff = (intptr_t)seq - (intptr_t)pos;
                if (diff == 0) {
                    if (enqueue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                        break;
                    }
                } else {
                    if (diff < 0) {
                        return false;
                    } else {
                        pos = enqueue_pos.load(std::memory_order_relaxed);
                    }
                }
            }

            cell->storage.emplace(data);
            cell->sequence.store(pos + 1, std::memory_order_release);
            return true;
        }

        bool dequeue(T &data)
        {
            const auto buffer_mask = buffer.size() - 1;
            Cell *cell = nullptr;

            size_t pos = dequeue_pos.load(std::memory_order_relaxed);
            for (;;) {
                cell = &buffer[pos & buffer_mask];
                size_t seq = cell->sequence_.load(std::memory_order_acquire);
                intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

                if (diff == 0)
                {
                    if (dequeue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                        break;
                    }
                } else {
                    if (diff < 0) {
                        return false;
                    } else {
                        pos = dequeue_pos.load(std::memory_order_relaxed);
                    }
                }
            }

            data = std::move(*cell->storage.ptr());
            cell->storage.destroy();
            cell->sequence_.store(pos + buffer_mask + 1, std::memory_order_release);
            return true;
        }
    };
}

namespace lib {
    template <class T>
    struct TypeName<lockfree::MPMCQueue<T>>
    {
        constexpr static inline StaticString name = "lib::lockfree::MPMCQueue<" + type_name<T> + ">";
    };
}
