#include "Memory_Manager.hpp"

#include "Backend/Logging/Logger_Manager.hpp"

#include <Windows.h>

#include <algorithm>
#include <sstream>

namespace Devilz::Backend
{
namespace
{
std::string WideToUtf8(const wchar_t* value)
{
    if (!value || !*value)
        return {};

    const int required = ::WideCharToMultiByte(
        CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1