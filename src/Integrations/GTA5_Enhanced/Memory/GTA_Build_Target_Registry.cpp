#include "GTA_Build_Target_Registry.hpp"

#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
GTA_Build_Target_Registry::GTA_Build_Target_Registry()
{
    GTA_Build_Target_Set current{};
    current.fingerprint = 0x6A4F97F605B81000ULL;
    current.targets = {
        {GTA_Runtime_Target_Id::GameState, "GameState", "GTA5_Enhanced.exe", {}, {}, true, GTA_Target_Candidate_Kind::Unknown},
        {GTA_Runtime_Target_Id::FrameCount, "FrameCount", "GTA5_Enhanced.exe", {}, {}, true, GTA_Target_Candidate_Kind::Unknown},
        {
            GTA_Runtime_Target_Id::ScriptGlobals,
            "ScriptGlobals",
            "GTA5_Enhanced.exe",
            "48 8B 8E B8 00 00 00 48 8D 15 ? ? ? ? 49 89 D8",
            {
                {GTA_Address_Resolve_Op_Type::Add, 7},
                {GTA_Address_Resolve_Op_Type::RipRelative32, 3}
            },
            true,
            GTA_Target_Candidate_Kind::DirectData
        },
        {
            GTA_Runtime_Target_Id::ProgramTable,
            "ProgramTable",
            "GTA5_Enhanced.exe",
            "48 C7 84 C8 D8 00 00 00 00 00 00 00",
            {},
            true,
            GTA_Target_Candidate_Kind::CodeSite
        },
        {
            GTA_Runtime_Target_Id::ScriptThreads,
            "ScriptThreads",
            "GTA5_Enhanced.exe",
            "48 8B 05 ? ? ? ? 48 89 34 F8 48 FF C7 48 39 FB 75 97",
            {
                {GTA_Address_Resolve_Op_Type::RipRelative32, 3}
            },
            true,
            GTA_Target_Candidate_Kind::PointerStorage
        },
        {
            GTA_Runtime_Target_Id::InitNativeTables,
            "InitNativeTables",
            "GTA5_Enhanced.exe",
            "EB 2A 0F 1F 40 00 48 8B 54 17 10",
            {
                {GTA_Address_Resolve_Op_Type::Add, -0x2A}
            },
            true,
            GTA_Target_Candidate_Kind::CodeSite
        },
        {
            GTA_Runtime_Target_Id::NativeTable,
            "NativeTable",
            "GTA5_Enhanced.exe",
            {},
            {},
            false,
            GTA_Target_Candidate_Kind::Unknown
        }
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
