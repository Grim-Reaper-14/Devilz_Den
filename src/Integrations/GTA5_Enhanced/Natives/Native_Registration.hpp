#pragma once
#include "Native_Hash.hpp"
#include "Backend/Memory/Pointer.hpp"
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced {
struct Native_Registration {
    Native_Hash hash{};
    Backend::Pointer handler{};
    bool validated = false;
    std::uint64_t callCount = 0;
};
}
