#include "GTA_Pointer_Validator.hpp"

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
GTA_Runtime_Target_Status MakeStatus(
    GTA_Runtime_Target_Id id,
    const char* name,
    bool present,
    bool required = true)
{
    GTA_Runtime_Target_Status status{};
    status.id = id;
    status.required = required;
    status.name = name;
    status.state = present ? GTA_Runtime_Target_State::Located : GTA_Runtime_Target_State::Missing;
    status.detail = present
        ? "Pointer resolved; semantic validation is still required"
        : "Required pointer has not been resolved for this build";
    return status;
}
}

std::vector<GTA_Runtime_Target_Status> GTA_Pointer_Validator::Validate(const GTA_Pointers& pointers)
{
    std::vector<GTA_Runtime_Target_Status> statuses;
    statuses.reserve(7);
    statuses.push_back(MakeStatus(GTA_Runtime_Target_Id::GameState, "GameState", !pointers.gameState.IsNull()));
    statuses.push_back(MakeStatus(GTA_Runtime_Target_Id::FrameCount, "FrameCount", !pointers.frameCount.IsNull()));
    statuses.push_back(MakeStatus(GTA_Runtime_Target_Id::ScriptGlobals, "ScriptGlobals", !pointers.scriptGlobals.IsNull()));
    statuses.push_back(MakeStatus(GTA_Runtime_Target_Id::ProgramTable, "ProgramTable", !pointers.programTable.IsNull()));
    statuses.push_back(MakeStatus(GTA_Runtime_Target_Id::ScriptThreads, "ScriptThreads", !pointers.scriptThreads.IsNull()));
    statuses.push_back(MakeStatus(GTA_Runtime_Target_Id::InitNativeTables, "InitNativeTables", !pointers.initNativeTables.IsNull()));
    statuses.push_back(MakeStatus(GTA_Runtime_Target_Id::NativeTable, "NativeTable", !pointers.nativeTable.IsNull()));
    return statuses;
}
}
