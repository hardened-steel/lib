#pragma once
#include <thread>

namespace lib {
    class Asio
    {
        void* handle = nullptr;

    public:
        Asio(std::size_t max_threads = std::thread::hardware_concurrency());
        ~Asio() noexcept;
    
    public:
        void attach(void* handle);
        void remove(void* handle);
    };
}
