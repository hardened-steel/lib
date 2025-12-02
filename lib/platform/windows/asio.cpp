#include "asio.hpp"
#include <stdexcept>

#define NOMINMAX
#include <Windows.h>

namespace lib {

    using nt_set_info_fn = LONG (NTAPI*)(HANDLE, ULONG_PTR*, void*, ULONG, ULONG);
    const nt_set_info_fn nt_set_info = [] {
        if (HMODULE h = GetModuleHandleA("NTDLL.DLL")) {
            return reinterpret_cast<nt_set_info_fn>(GetProcAddress(h, "NtSetInformationFile"));
        }
        throw std::runtime_error("error load NtSetInformationFile function");
    }();

    Asio::Asio(std::size_t max_threads)
    : handle(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, max_threads))
    {

    }

    Asio::~Asio() noexcept
    {
        CloseHandle(handle);
    }

    void Asio::remove(void* handle)
    {
        constexpr ULONG FileReplaceCompletionInformation = 61; // NOLINT
        ULONG_PTR iosb[2] = {0, 0};                    // NOLINT
        void* info[2] = {nullptr, nullptr};            // NOLINT
        nt_set_info(handle, iosb, &info, sizeof(info), FileReplaceCompletionInformation);
    }

}
