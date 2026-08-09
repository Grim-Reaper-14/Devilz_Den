#pragma once

#include <Windows.h>

namespace Devilz::DLL_Bootstrap
{
[[nodiscard]] bool Attach(HMODULE module) noexcept;
void RequestUnload() noexcept;
[[nodiscard]] bool Running() noexcept;
}
