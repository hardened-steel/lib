#include "file.hpp"
#include <windows.h>

using namespace lib;

File::File(const std::filesystem::path& path)
: handle(nullptr)
{
    handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
}
