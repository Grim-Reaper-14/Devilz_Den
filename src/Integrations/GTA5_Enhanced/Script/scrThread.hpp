#pragma once

#include "Backend/Memory/Pointer.hpp"

#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class scrThreadState : std::uint8_t
{
    Unknown,
    Idle,
    Running,
    Killed,
    Paused
};

struct scrThreadContext
{
    std::uint32_t threadId = 0;
    std::uint32_t scriptHash = 0;
    std::uint32_t instructionPointer = 0;
    std::uint32_t framePointer = 0;
    std::uint32_t stackPointer = 0;
    scrThreadState state = scrThreadState::Unknown;
};

struct scrThread
{
    scrThreadContext context{};
    Devilz::Backend::Pointer stack;

    [[nodiscard]] bool Valid() const noexcept { return context.threadId != 0; }
    [[nodiscard]] bool Running() const noexcept { return context.state == scrThreadState::Running; }
};
}
