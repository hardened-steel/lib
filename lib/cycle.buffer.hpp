#pragma once
#include <iterator>
#include <array>
#include <atomic>
#include <lib/raw.storage.hpp>

namespace lib {

    template<class T, std::size_t N>
    class CycleBuffer;

    struct CycleBufferEndIterator {};
    constexpr inline CycleBufferEndIterator cycle_buffer_end = {};

    template<class T, std::size_t N>
    class CycleBufferIterator
    {
    protected:
        T*          buffer;
        std::size_t position;
        std::size_t size;
    public:
        using iterator_category = std::random_access_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;
        using reference         = T&;
    public:
        constexpr CycleBufferIterator(T* buffer, std::size_t position, std::size_t size) noexcept // NOLINT
        : buffer(buffer), position(position), size(size)
        {}
        CycleBufferIterator(const CycleBufferIterator&) = default;
        CycleBufferIterator& operator=(const CycleBufferIterator&) = default;
    public:
        reference operator*() const noexcept
        {
            return buffer[position];
        }
        pointer operator->() const noexcept
        {
            return &buffer[position];
        }
        CycleBufferIterator& operator++() noexcept
        {
            position += 1;
            if(position == N) {
                position = 0;
            }
            size -= 1;
            return *this;
        }
        CycleBufferIterator& operator--() noexcept
        {
            if(position == 0) {
                position = N;
            }
            position -= 1;
            size += 1;
            return *this;
        }
        CycleBufferIterator& operator+=(std::size_t index) noexcept
        {
            position = (0 + position + index) % N;
            size -= index;
            return *this;
        }
        CycleBufferIterator& operator-=(std::size_t index) noexcept
        {
            position = (N + position - index) % N;
            size += index;
            return *this;
        }
        CycleBufferIterator operator++(int) noexcept
        {
            CycleBufferIterator temp = *this;
            ++(*this);
            return temp;
        }
        CycleBufferIterator operator--(int) noexcept
        {
            CycleBufferIterator temp = *this;
            --(*this);
            return temp;
        }
        constexpr friend bool operator==(const CycleBufferIterator& it, const CycleBufferEndIterator&) noexcept
        {
            return it.size == 0;
        }
        constexpr friend bool operator==(const CycleBufferEndIterator&, const CycleBufferIterator& it) noexcept
        {
            return it.size == 0;
        }
        constexpr friend bool operator!=(const CycleBufferIterator& it, const CycleBufferEndIterator&) noexcept
        {
            return it.size != 0;
        }
        constexpr friend bool operator!=(const CycleBufferEndIterator&, const CycleBufferIterator& it) noexcept
        {
            return it.size != 0;
        }
    };

    template<class T, std::size_t N>
    class CycleBuffer
    {
        alignas(64) std::atomic<std::size_t> m_recv_index{0};
        alignas(64) std::atomic<std::size_t> m_send_index{0};

        using Storage = RawStorage<T>;
        std::array<Storage, N + 1> array;
    public:
        T recv() noexcept
        {
            const auto recv_index = m_recv_index.load();
            auto& storage = array[recv_index];
            auto* ptr = storage.ptr();
            T value = std::move(*ptr);
            storage.destroy();

            auto new_recv_index = recv_index + 1;
            if (new_recv_index == array.size()) {
                new_recv_index = 0;
            }
            m_recv_index.store(new_recv_index);
            return value;
        }
        bool rpoll() const noexcept
        {
            return rsize() > 0;
        }
        void send(T value) noexcept
        {
            const auto send_index = m_send_index.load();
            auto new_send_index = send_index + 1;
            if (new_send_index == array.size()) {
                new_send_index = 0;
            }
            array[send_index].emplace(std::move(value));
            m_send_index.store(new_send_index);
        }
        bool spoll() const noexcept
        {
            return wsize() > 0;
        }
    public:
        std::size_t rsize() const noexcept
        {
            const auto send_index = m_send_index.load();
            const auto recv_index = m_recv_index.load();
            if (send_index >= recv_index) {
                return send_index - recv_index;
            } else {
                return array.size() - (recv_index - send_index);
            }
        }
        std::size_t wsize() const noexcept
        {
            const auto send_index = m_send_index.load();
            const auto recv_index = m_recv_index.load();
            if (send_index >= recv_index) {
                return array.size() - (send_index - recv_index) - 1;
            } else {
                return recv_index - send_index - 1;
            }
        }
        void clear() noexcept
        {
            while(rpoll()) {
                recv();
            }
        }
    public:
        
        CycleBufferIterator<const T, N + 1> begin() const noexcept
        {
            return {array[0].ptr(), m_recv_index.load(std::memory_order_relaxed), rsize()};
        }
        auto end() const noexcept
        {
            return cycle_buffer_end;
        }

        auto cbegin() const noexcept
        {
            return begin();
        }
        auto cend() const noexcept
        {
            return end();
        }

        CycleBufferIterator<T, N + 1> begin() noexcept
        {
            return {array[0].ptr(), m_recv_index.load(std::memory_order_relaxed), rsize()};
        }
        auto end() noexcept
        {
            return cycle_buffer_end;
        }

    private:
        static void copy(const CycleBuffer& from, CycleBuffer& to)
        {
            if(auto count = from.count.load(std::memory_order_relaxed)) {
                auto tail = from.tail;
                while(count--) {
                    auto* ptr = from.array[tail].ptr();
                    to.send(*ptr);
                    tail += 1;
                    if(tail == N) {
                        tail = 0;
                    }
                }
            }
        }
    public:
        constexpr static std::size_t capacity() noexcept
        {
            return N + 1;
        }
        CycleBuffer() = default;
        CycleBuffer(const CycleBuffer& other)
        {
            copy(other, *this);
        }
        CycleBuffer(CycleBuffer&& other) noexcept
        {
            while(other.rpoll()) {
                send(other.recv());
            }
        }
        CycleBuffer& operator=(const CycleBuffer& other)
        {
            if(this != &other) {
                clear();
                copy(other, *this);
            }
            return *this;
        }
        CycleBuffer& operator=(CycleBuffer&& other) noexcept
        {
            if(this != &other) {
                clear();
                while(other.rpoll()) {
                    send(other.recv());
                }
            }
            return *this;
        }
        ~CycleBuffer() noexcept
        {
            clear();
        }
    };
}
