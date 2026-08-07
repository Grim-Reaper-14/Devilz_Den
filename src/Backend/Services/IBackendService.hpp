#pragma once

#include "Backend/Error/Result.hpp"

#include <cstdint>
#include <string_view>

namespace Devilz::Backend
{
enum class ServiceState : std::uint8_t
{
    Created,
    Initializing,
    Running,
    Paused,
    Stopping,
    Stopped,
    Failed
};

class IBackendService
{
public:
    virtual ~IBackendService() = default;

    [[nodiscard]] virtual std::string_view Name() const noexcept = 0;
    [[nodiscard]] virtual ServiceState State() const noexcept = 0;

    virtual Result<void> Initialize() = 0;
    virtual Result<void> Start() = 0;
    virtual Result<void> Stop() = 0;
    virtual void Shutdown() noexcept = 0;
};
}
