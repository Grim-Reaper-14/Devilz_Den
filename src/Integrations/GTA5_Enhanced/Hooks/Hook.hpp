#pragma once
#include "Backend/Error/Result.hpp"
#include <cstdint>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced {
enum class Hook_State : std::uint8_t { Created, Resolving, Ready, Installing, Installed, Removing, Removed, Failed };
class Hook {
public:
    virtual ~Hook() = default;
    [[nodiscard]] virtual std::string_view Name() const noexcept = 0;
    [[nodiscard]] virtual Hook_State State() const noexcept = 0;
    virtual Backend::Result<void> Install() = 0;
    virtual Backend::Result<void> Remove() = 0;
};
}
