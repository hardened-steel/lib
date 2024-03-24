#pragma once
#include <iterator>
#include <array>
#include <atomic>

namespace lib {

    template<class T, std::size_t N>
    class CycleBuffer;

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
        constexpr CycleBufferIterator(T* buffer, std::size_t position, std::size_t size) noexcept
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
        constexpr friend bool operator==(const CycleBufferIterator& a, const CycleBufferIterator& b) noexcept
        {
            return a.buffer == b.buffer && a.position == b.position && a.size == b.size;
        }
        constexpr friend bool operator!=(const CycleBufferIterator& a, const CycleBufferIterator& b) noexcept
        {
            return a.buffer != b.buffer || a.position != b.position || a.size != b.size;
        }
    };

    template<class T, std::size_t N>
    class CycleBuffer
    {
    public:
        T recv() noexcept
        {
            assert(rpoll());
            assert(rsize() <= N);

            auto tail = this->tail.load();
            auto ptr = reinterpret_cast<T*>(&array[tail]);
            T value = std::move(*ptr);
            ptr->~T();
            tail += 1;
            if(tail == N) {
                tail = 0;
            }
            this->tail = tail;
            --count;
            return value;
        }
        bool rpoll() const noexcept
        {
            return rsize() > 0;
        }
        void send(T value) noexcept
        {
            assert(spoll());
            assert(wsize() <= N);

            auto head = this->head.load();
            new(&array[head]) T(std::move(value));
            head += 1;
            if(head == N) {
                head = 0;
            }
            this->head = head;
            ++count;
        }
        bool spoll() const noexcept
        {
            return wsize() > 0;
        }
    public:
        std::size_t rsize() const noexcept
        {
            return count.load(std::memory_order_relaxed);
        }
        std::size_t wsize() const noexcept
        {
            return N - count.load(std::memory_order_relaxed);
        }
        void clear() noexcept
        {
            if(auto count = this->count.load(std::memory_order_relaxed)) {
                auto tail = this->tail.load(std::memory_order_relaxed);
                while(count--) {
                    auto ptr = reinterpret_cast<T*>(&array[tail]);
                    ptr->~T();
                    tail += 1;
                    if(tail == N) {
                        tail = 0;
                    }
                }
            }
        }
    public:
        
        CycleBufferIterator<const T, N> begin() const noexcept
        {
            return {reinterpret_cast<const T*>(array.data()), tail, count.load(std::memory_order_relaxed)};
        }
        CycleBufferIterator<const T, N> end() const noexcept
        {
            return {reinterpret_cast<const T*>(array.data()), head, 0};
        }

        auto cbegin() const noexcept
        {
            return begin();
        }
        auto cend() const noexcept
        {
            return end();
        }

        CycleBufferIterator<T, N> begin() noexcept
        {
            return {reinterpret_cast<T*>(array.data()), tail, count.load(std::memory_order_relaxed)};
        }
        CycleBufferIterator<T, N> end() noexcept
        {
            return {reinterpret_cast<T*>(array.data()), head, 0};
        }

        auto rbegin() noexcept
        {
            return std::make_reverse_iterator(end());
        }
        auto rend() noexcept
        {
            return std::make_reverse_iterator(begin());
        }

        auto rbegin() const noexcept
        {
            return std::make_reverse_iterator(end());
        }
        auto rend() const noexcept
        {
            return std::make_reverse_iterator(begin());
        }

        auto crbegin() const noexcept
        {
            return rbegin();
        }
        auto crend() const noexcept
        {
            return rend();
        }

        struct MReversed
        {
            CycleBuffer& buffer;

            auto begin() noexcept
            {
                return buffer.rbegin();
            }
            auto end() noexcept
            {
                return buffer.rend();
            }
        };

        MReversed reverse() noexcept
        {
            return {*this};
        }

        struct CReversed
        {
            const CycleBuffer& buffer;

            auto begin() noexcept
            {
                return buffer.rbegin();
            }
            auto end() noexcept
            {
                return buffer.rend();
            }
        };
        
        CReversed reverse() const noexcept
        {
            return {*this};
        }
        
    private:
        using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;
        
        std::array<Storage, N>   array;
        std::atomic<std::size_t> head  {0};
        std::atomic<std::size_t> tail  {0};
        std::atomic<std::size_t> count {0};
    private:
        static void copy(const CycleBuffer& from, CycleBuffer& to)
        {
            if(auto count = from.count.load(std::memory_order_relaxed)) {
                auto tail = from.tail;
                while(count--) {
                    auto ptr = reinterpret_cast<T*>(&from.array[tail]);
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
            return N;
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
        CycleBuffer& operator=(CycleBuffer&& other)
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
