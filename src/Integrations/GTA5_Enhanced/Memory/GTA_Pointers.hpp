#pragma once
#include "Backend/Memory/Pointer.hpp"

namespace Devilz::Integrations::GTA5_Enhanced {
struct GTA_Pointers {
    Backend::Pointer nativeTable{};
    Backend::Pointer gameState{};
    Backend::Pointer scriptGlobals{};
    Backend::Pointer frameCount{};
    [[nodiscard]] bool Ready() const noexcept { return nativeTable && gameState; }
};
}
