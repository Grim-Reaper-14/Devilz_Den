#pragma once
#include <cstddef>

namespace Devilz::Integrations::GTA5_Enhanced {
struct GTA_Offsets {
    std::ptrdiff_t gameState = 0;
    std::ptrdiff_t frameCount = 0;
    std::ptrdiff_t scriptGlobals = 0;
};
}
