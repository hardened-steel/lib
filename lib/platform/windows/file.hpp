#pragma once
#include <filesystem>

namespace lib {
    class File
    {
        void* handle;
    public:
        File(const std::filesystem::path& path);
    };
}
