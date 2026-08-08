#pragma once
#include "Backend/Memory/Pointer.hpp"
#include <string>

namespace Devilz::Integrations::GTA5_Enhanced {
struct Hook_Target {
    std::string name;
    std::string module;
    Backend::Pointer address{};
    [[nodiscard]] bool Valid() const noexcept { return static_cast<bool>(address); }
};
}
