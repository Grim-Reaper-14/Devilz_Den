#include "GTA_Build_Target_Registry.hpp"

#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
GTA_Build_Target_Registry::GTA_Build_Target_Registry()
{
    GTA_Build_Target_Set current{};
    current.fingerprint = 0x6A4F97F605B81000ULL;
    current.targets = {
        {GTA_Runtime_Target_Id::GameState, "GameState", "GTA5_Enhanced.exe", {}, {}, true},
        {GTA_Runtime_Target_Id::FrameCount, "FrameCount", "GTA5_Enhanced.exe", {}, {}, true},
        {GTA_Runtime_Target_Id::ScriptGlobals, "ScriptGlobals", "GTA5_Enhanced.exe", {}, {}, true},
        {GTA_Runtime_Target_Id::ProgramTable, "ProgramTable", "GTA5_Enhanced.exe", {}, {}, true},
        {GTA_Runtime_Target_Id::ScriptThreads, "ScriptThreads", "GTA5_Enhanced.exe", {}, {}, true},
        {GTA_Runtime_Target_Id::InitNativeTables, "InitNativeTables", "GTA5_Enhanced.exe", {}, {}, true},
        {GTA_Runtime_Target_Id::NativeTable, "NativeTable", "GTA5_Enhanced.exe", {}, {}, true}
    };
    Register(std::move(current));
}

void GTA_Build_Target_Registry::Register(GTA_Build_Target_Set set)
{
    if (set.fingerprint == 0)
        return;
    m_sets.insert_or_assign(set.fingerprint, std::move(set));
}

std::optional<GTA_Build_Target_Set> GTA_Build_Target_Registry::Find(std::uint64_t fingerprint) const
{
    const auto it = m_sets.find(fingerprint);
    if (it == m_sets.end())
        return std::nullopt;
    return it->second;
}
}
