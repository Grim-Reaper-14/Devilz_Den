#pragma once

#include <cstddef>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Native_Call_Context;

using GTA_Native_Hash = std::uint64_t;
using GTA_Native_Handler = void (*)(GTA_Native_Call_Context*);

struct GTA_Native_Program_Bootstrap
{
    std::byte pad00[0x2C]{};
    std::uint32_t nativeCount = 0;
    std::byte pad30[0x10]{};
    GTA_Native_Handler* nativeEntrypoints = nullptr;
    std::byte pad48[0x38]{};
};

using GTA_Init_Native_Tables = void (*)(GTA_Native_Program_Bootstrap*);

static_assert(offsetof(GTA_Native_Program_Bootstrap, nativeCount) == 0x2C);
static_assert(offsetof(GTA_Native_Program_Bootstrap, nativeEntrypoints) == 0x40);
static_assert(sizeof(GTA_Native_Program_Bootstrap) == 0x80);
}
