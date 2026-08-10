#pragma once

#include <cstdint>
#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Runtime_Target_Id : std::uint8_t
{
    GameState,
    FrameCount,
    ScriptGlobals,
    ProgramTable,
    ScriptThreads,
    InitNativeTables,
    NativeTable
};

enum class GTA_Runtime_Target_State : std::uint8_t
{
    Unknown,
    Missing,
    Located,
    StructurallyValidated,
    Validated,
    Failed
};

struct GTA_Runtime_Target_Status
{
    GTA_Runtime_Target_Id id = GTA_Runtime_Target_Id::GameState;
    GTA_Runtime_Target_State state = GTA_Runtime_Target_State::Unknown;
    bool required = true;
    std::string name;
    std::string detail;
};
}
