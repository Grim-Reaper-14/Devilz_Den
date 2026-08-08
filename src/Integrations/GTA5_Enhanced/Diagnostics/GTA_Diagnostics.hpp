#pragma once

#include "../Memory/GTA_Pointers.hpp"
#include "../Runtime/Build_Info.hpp"
#include "../Runtime/GTA_Runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Diagnostic_Snapshot
{
    std::uint64_t buildFingerprint = 0;
    GTA_Runtime_State runtimeState = GTA_Runtime_State::Created;
    bool corePointersReady = false;
    bool scriptPointersReady = false;
    bool nativeBootstrapReady = false;
    std::size_t programs = 0;
    std::size_t scriptThreads = 0;
    std::size_t nativeHandlers = 0;
    std::string lastFailure;
};

class GTA_Diagnostics final
{
public:
    void SetFailure(std::string failure);
    void ClearFailure() noexcept;
    [[nodiscard]] GTA_Diagnostic_Snapshot Capture(GTA_Runtime& runtime) const;

private:
    std::string m_lastFailure;
};
}
