#include "GTA_Stats_Extension.hpp"

#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Types/Stats/GTA_Stats_Manager.hpp"

#include <Windows.h>

#include <array>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr GTA_Native_Hash StatSetInt = 0x1164A75E490C27B6ULL;
constexpr GTA_Native_Hash StatSetFloat = 0x4F8678C02360C3D2ULL;
constexpr GTA_Native_Hash StatSetBool = 0xF1D0B0CE940F620DULL;
constexpr GTA_Native_Hash StatSetString = 0xFE0BEB152470B0B8ULL;
constexpr std::array<int, 13> StatsMgrPattern{
    0x89, 0x6C, 0x24, 0x28, 0x48, 0x8D, 0x0D, -1, -1, -1, -1, 0x48, 0x8D
};

GTA_Stats_Manager_View* g_manager = nullptr;
bool g_scanned = false;

[[nodiscard]] bool Readable(std::uintptr_t address, std::size_t size) noexcept
{
    if (!address || !size || address > (std::numeric_limits<std::uintptr_t>::max)() - size)
        return false;
    MEMORY_BASIC_INFORMATION memory{};
    if (!::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)))
        return false;
    const auto p = memory.Protect & 0xFFU;
    const bool readable = p == PAGE_READONLY || p == PAGE_READWRITE || p == PAGE_WRITECOPY ||
        p == PAGE_EXECUTE_READ || p == PAGE_EXECUTE_READWRITE || p == PAGE_EXECUTE_WRITECOPY;
    const auto begin = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    const auto end = begin + memory.RegionSize;
    return memory.State == MEM_COMMIT && !(memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
        readable && address >= begin && address + size <= end;
}

[[nodiscard]] bool Executable(std::uintptr_t address) noexcept
{
    MEMORY_BASIC_INFORMATION memory{};
    if (!address || !::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)))
        return false;
    const auto p = memory.Protect & 0xFFU;
    return memory.State == MEM_COMMIT && !(memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
        (p == PAGE_EXECUTE || p == PAGE_EXECUTE_READ || p == PAGE_EXECUTE_READWRITE || p == PAGE_EXECUTE_WRITECOPY);
}

[[nodiscard]] bool Slot(const GTA_Stat_Data* stat, std::size_t offset) noexcept
{
    if (!stat || !Readable(reinterpret_cast<std::uintptr_t>(stat), sizeof(GTA_Stat_Data)))
        return false;
    std::uintptr_t vtable = 0, function = 0;
    std::memcpy(&vtable, static_cast<const void*>(stat), sizeof(vtable));
    if (!Readable(vtable + offset, sizeof(function)))
        return false;
    std::memcpy(&function, reinterpret_cast<const void*>(vtable + offset), sizeof(function));
    return Executable(function);
}

[[nodiscard]] bool PatternAt(const std::uint8_t* bytes) noexcept
{
    for (std::size_t i = 0; i < StatsMgrPattern.size(); ++i)
        if (StatsMgrPattern[i] >= 0 && bytes[i] != static_cast<std::uint8_t>(StatsMgrPattern[i]))
            return false;
    return true;
}

[[nodiscard]] GTA_Stats_Manager_View* ResolveManager() noexcept
{
    if (g_manager || g_scanned)
        return g_manager;
    g_scanned = true;
    const auto module = ::GetModuleHandleW(L"GTA5_Enhanced.exe");
    if (!module)
        return nullptr;
    const auto base = reinterpret_cast<std::uintptr_t>(module);
    if (!Readable(base, sizeof(IMAGE_DOS_HEADER)))
        return nullptr;
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
        return nullptr;
    const auto ntAddress = base + static_cast<std::uintptr_t>(dos->e_lfanew);
    if (!Readable(ntAddress, sizeof(IMAGE_NT_HEADERS64)))
        return nullptr;
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(ntAddress);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return nullptr;

    std::uintptr_t match = 0;
    std::size_t matches = 0;
    const auto* sections = IMAGE_FIRST_SECTION(nt);
    for (std::uint16_t sectionIndex = 0; sectionIndex < nt->FileHeader.NumberOfSections; ++sectionIndex) {
        const auto& section = sections[sectionIndex];
        if (!(section.Characteristics & IMAGE_SCN_MEM_EXECUTE))
            continue;
        const auto start = base + section.VirtualAddress;
        const auto size = static_cast<std::size_t>(section.Misc.VirtualSize);
        if (size < StatsMgrPattern.size() || !Readable(start, size))
            continue;
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(start);
        for (std::size_t i = 0; i + StatsMgrPattern.size() <= size; ++i) {
            if (!PatternAt(bytes + i))
                continue;
            match = start + i;
            if (++matches > 1)
                return nullptr;
        }
    }
    if (matches != 1 || !Readable(match + 7, sizeof(std::int32_t)))
        return nullptr;
    std::int32_t displacement = 0;
    std::memcpy(&displacement, reinterpret_cast<const void*>(match + 7), sizeof(displacement));
    const auto resolved = static_cast<std::intptr_t>(match + 11) + displacement;
    if (resolved <= 0)
        return nullptr;
    auto* manager = reinterpret_cast<GTA_Stats_Manager_View*>(static_cast<std::uintptr_t>(resolved));
    if (!Readable(reinterpret_cast<std::uintptr_t>(manager), sizeof(*manager)))
        return nullptr;
    g_manager = manager;
    return manager;
}

[[nodiscard]] bool ManagerReady(const GTA_Stats_Manager_View* manager) noexcept
{
    if (!manager || !Readable(reinterpret_cast<std::uintptr_t>(manager), sizeof(*manager)) ||
        !manager->initialized || manager->stats.size > manager->stats.capacity)
        return false;
    return manager->stats.size == 0 || (manager->stats.data &&
        Readable(reinterpret_cast<std::uintptr_t>(manager->stats.data),
            static_cast<std::size_t>(manager->stats.size) * sizeof(GTA_Stat_Map)));
}

[[nodiscard]] char Lower(char value) noexcept
{
    return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value;
}

[[nodiscard]] std::string_view Trim(std::string_view value) noexcept
{
    auto space = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!value.empty() && space(value.front())) value.remove_prefix(1);
    while (!value.empty() && space(value.back())) value.remove_suffix(1);
    return value;
}

[[nodiscard]] bool StartsI(std::string_view value, std::string_view prefix) noexcept
{
    if (value.size() < prefix.size()) return false;
    for (std::size_t i = 0; i < prefix.size(); ++i)
        if (Lower(value[i]) != Lower(prefix[i])) return false;
    return true;
}

[[nodiscard]] std::uint32_t Joaat(std::string_view text) noexcept
{
    std::uint32_t hash = 0;
    for (char c : text) {
        hash += static_cast<std::uint8_t>(Lower(c));
        hash += hash << 10U;
        hash ^= hash >> 6U;
    }
    hash += hash << 3U;
    hash ^= hash >> 11U;
    return hash + (hash << 15U);
}

[[nodiscard]] int CharacterIndex(GTA_Stats_Manager_View& manager) noexcept
{
    auto* stat = manager.GetStat(Joaat("MPPLY_LAST_MP_CHAR"));
    if (!stat || !Slot(stat, 0xC0) || !Slot(stat, 0x60)) return 0;
    return stat->GetInt() == 1 ? 1 : 0;
}

struct ResolvedStat { std::string name; std::uint32_t hash = 0; GTA_Stat_Data* data = nullptr; };

[[nodiscard]] ResolvedStat ResolveStat(GTA_Stats_Manager_View& manager, std::string_view input)
{
    input = Trim(input);
    if (!input.empty() && input.front() == '$') input = Trim(input.substr(1));
    if (input.empty() || input.size() > 127) return {};
    ResolvedStat result{std::string(input)};
    const int character = CharacterIndex(manager);
    if (result.name.size() > 3 && StartsI(result.name, "MPX"))
        result.name[2] = static_cast<char>('0' + character);
    result.hash = Joaat(result.name);
    result.data = manager.GetStat(result.hash);
    if (result.data) return result;
    const bool explicitMp = result.name.size() > 3 && Lower(result.name[0]) == 'm' && Lower(result.name[1]) == 'p' &&
        (result.name[2] == '0' || result.name[2] == '1');
    if (explicitMp) return result;
    result.name = (character == 0 ? "MP0_" : "MP1_") + result.name;
    result.hash = Joaat(result.name);
    result.data = manager.GetStat(result.hash);
    return result;
}

[[nodiscard]] bool ReadString(const char* text, std::string& output)
{
    output.clear();
    if (!text) return true;
    const auto address = reinterpret_cast<std::uintptr_t>(text);
    for (std::size_t i = 0; i < 255; ++i) {
        if (!Readable(address + i, 1)) return false;
        if (!text[i]) return true;
        output.push_back(text[i]);
    }
    return false;
}

[[nodiscard]] bool ReadValue(GTA_Stat_Data& data, GTA_Stat_Data::Type type, std::string& out)
{
    switch (type) {
    case GTA_Stat_Data::Type::Int: if (!Slot(&data, 0x60)) return false; out = std::to_string(data.GetInt()); return true;
    case GTA_Stat_Data::Type::Int64: if (!Slot(&data, 0x68)) return false; out = std::to_string(data.GetInt64()); return true;
    case GTA_Stat_Data::Type::Float: if (!Slot(&data, 0x70)) return false; out = std::to_string(data.GetFloat()); return true;
    case GTA_Stat_Data::Type::Bool: if (!Slot(&data, 0x78)) return false; out = data.GetBool() ? "true" : "false"; return true;
    case GTA_Stat_Data::Type::UInt8: if (!Slot(&data, 0x80)) return false; out = std::to_string(data.GetUInt8()); return true;
    case GTA_Stat_Data::Type::UInt16: if (!Slot(&data, 0x88)) return false; out = std::to_string(data.GetUInt16()); return true;
    case GTA_Stat_Data::Type::UInt32: if (!Slot(&data, 0x90)) return false; out = std::to_string(data.GetUInt32()); return true;
    case GTA_Stat_Data::Type::UInt64: if (!Slot(&data, 0x98)) return false; out = std::to_string(data.GetUInt64()); return true;
    case GTA_Stat_Data::Type::String: if (!Slot(&data, 0xA0)) return false; return ReadString(data.GetString(), out);
    default: return false;
    }
}

[[nodiscard]] bool Writable(GTA_Stat_Data::Type type) noexcept
{
    return type == GTA_Stat_Data::Type::Int || type == GTA_Stat_Data::Type::Float || type == GTA_Stat_Data::Type::Bool ||
        type == GTA_Stat_Data::Type::UInt8 || type == GTA_Stat_Data::Type::UInt16 || type == GTA_Stat_Data::Type::UInt32 ||
        type == GTA_Stat_Data::Type::String;
}

[[nodiscard]] bool ParseSigned(std::string_view text, std::int32_t& value) noexcept
{
    std::string copy(Trim(text));
    if (copy.empty()) return false;
    char* end = nullptr; errno = 0;
    const auto parsed = std::strtoll(copy.c_str(), &end, 10);
    if (errno == ERANGE || end != copy.c_str() + copy.size() || parsed < INT32_MIN || parsed > INT32_MAX) return false;
    value = static_cast<std::int32_t>(parsed); return true;
}

[[nodiscard]] bool ParseUnsigned(std::string_view text, std::uint32_t& value) noexcept
{
    std::string copy(Trim(text));
    if (copy.empty() || copy.front() == '-') return false;
    char* end = nullptr; errno = 0;
    const auto parsed = std::strtoull(copy.c_str(), &end, 10);
    if (errno == ERANGE || end != copy.c_str() + copy.size() || parsed > UINT32_MAX) return false;
    value = static_cast<std::uint32_t>(parsed); return true;
}

[[nodiscard]] bool WriteValue(GTA_Native_Manager& natives, std::uint32_t hash, GTA_Stat_Data::Type type,
    std::string_view text, std::string& value, std::string& error)
{
    if (type == GTA_Stat_Data::Type::Int) {
        std::int32_t v{}; if (!ParseSigned(text, v)) { error = "Expected a signed 32-bit integer"; return false; }
        value = std::to_string(v); if (!natives.InvokeHash<void>(StatSetInt, hash, v, true)) { error = "STAT_SET_INT handler unavailable"; return false; } return true;
    }
    if (type == GTA_Stat_Data::Type::UInt8 || type == GTA_Stat_Data::Type::UInt16 || type == GTA_Stat_Data::Type::UInt32) {
        std::uint32_t v{}; if (!ParseUnsigned(text, v)) { error = "Expected an unsigned integer"; return false; }
        if ((type == GTA_Stat_Data::Type::UInt8 && v > 0xFFU) || (type == GTA_Stat_Data::Type::UInt16 && v > 0xFFFFU)) { error = "UINT value is out of range"; return false; }
        std::int32_t nativeValue{}; std::memcpy(&nativeValue, &v, sizeof(v)); value = std::to_string(v);
        if (!natives.InvokeHash<void>(StatSetInt, hash, nativeValue, true)) { error = "STAT_SET_INT handler unavailable"; return false; }
        return true;
    }
    if (type == GTA_Stat_Data::Type::Float) {
        std::string copy(Trim(text)); char* end = nullptr; errno = 0; const float v = std::strtof(copy.c_str(), &end);
        if (copy.empty() || errno == ERANGE || end != copy.c_str() + copy.size() || !std::isfinite(v)) { error = "Expected a finite floating-point value"; return false; }
        value = std::to_string(v); if (!natives.InvokeHash<void>(StatSetFloat, hash, v, true)) { error = "STAT_SET_FLOAT handler unavailable"; return false; } return true;
    }
    if (type == GTA_Stat_Data::Type::Bool) {
        const auto t = Trim(text); bool v{};
        if (t == "1" || (t.size() == 4 && StartsI(t, "true"))) v = true; else if (t != "0" && !(t.size() == 5 && StartsI(t, "false"))) { error = "Expected true, false, 1, or 0"; return false; }
        value = v ? "true" : "false"; if (!natives.InvokeHash<void>(StatSetBool, hash, v, true)) { error = "STAT_SET_BOOL handler unavailable"; return false; } return true;
    }
    if (type == GTA_Stat_Data::Type::String) {
        if (text.size() > 255) { error = "String is limited to 255 characters"; return false; }
        value.assign(text); if (!natives.InvokeHash<void>(StatSetString, hash, value.c_str(), true)) { error = "STAT_SET_STRING handler unavailable"; return false; } return true;
    }
    error = "This stat type is read-only"; return false;
}

[[nodiscard]] GTA_Stat_Snapshot Execute(GTA_Native_Manager& natives, const GTA_Stats_State::Command& command)
{
    GTA_Stat_Snapshot result{}; result.requestId = command.id; result.requestedName = command.statName;
    auto* manager = ResolveManager();
    if (!ManagerReady(manager)) { result.status = GTA_Stat_Request_Status::RuntimeUnavailable; result.detail = "CStatsMgr is unavailable or not initialized"; return result; }
    const auto resolved = ResolveStat(*manager, command.statName); result.normalizedName = resolved.name; result.hash = resolved.hash;
    if (!resolved.data) { result.status = GTA_Stat_Request_Status::NotFound; result.detail = resolved.name.empty() ? "Enter a valid stat name" : "Stat not found in CStatsMgr"; return result; }
    if (!Slot(resolved.data, 0xC0)) { result.status = GTA_Stat_Request_Status::Failed; result.detail = "Stat object failed vtable validation"; return result; }
    result.type = resolved.data->GetType(); result.typeKnown = true; result.writeSupported = Writable(result.type);
    result.serverAuthoritative = resolved.data->IsServerAuthoritative(); result.controlledByNetShop = resolved.data->IsControlledByNetShop();
    if (command.kind == GTA_Stats_State::Command_Kind::Read) {
        result.status = ReadValue(*resolved.data, result.type, result.value) ? GTA_Stat_Request_Status::Succeeded : GTA_Stat_Request_Status::UnsupportedType;
        result.detail = result.status == GTA_Stat_Request_Status::Succeeded ? "Stat read on the GTA game thread" : "Stat type is not readable by this editor"; return result;
    }
    if (result.controlledByNetShop) { (void)ReadValue(*resolved.data, result.type, result.value); result.status = GTA_Stat_Request_Status::ControlledByNetShop; result.detail = "Write blocked: stat is controlled by Netshop"; return result; }
    if (!result.writeSupported) { (void)ReadValue(*resolved.data, result.type, result.value); result.status = GTA_Stat_Request_Status::UnsupportedType; result.detail = "Write blocked: no safe native path for this type"; return result; }
    std::string error;
    if (!WriteValue(natives, result.hash, result.type, command.value, result.value, error)) {
        result.status = StartsI(error, "Expected") || StartsI(error, "UINT") || StartsI(error, "String") ? GTA_Stat_Request_Status::InvalidValue : GTA_Stat_Request_Status::Failed;
        result.detail = std::move(error); return result;
    }
    result.status = GTA_Stat_Request_Status::Succeeded;
    result.detail = result.serverAuthoritative ? "Write dispatched; server-authoritative stat may be replaced" : "Write dispatched through the stat native on the GTA game thread";
    return result;
}
}

GTA_Stats_State& GTA_Stats_State::Instance() noexcept { static GTA_Stats_State state; return state; }

void GTA_Stats_State::RequestRead(std::string statName)
{
    std::scoped_lock lock(m_mutex); const auto id = m_nextRequestId++;
    m_pending = Command{id, Command_Kind::Read, std::move(statName), {}}; m_snapshot.requestId = id;
    m_snapshot.status = GTA_Stat_Request_Status::Queued; m_snapshot.requestedName = m_pending->statName;
    m_snapshot.detail = "Read queued for the GTA game thread"; ++m_snapshot.revision;
}

void GTA_Stats_State::RequestWrite(std::string statName, std::string value)
{
    std::scoped_lock lock(m_mutex); const auto id = m_nextRequestId++;
    m_pending = Command{id, Command_Kind::Write, std::move(statName), std::move(value)}; m_snapshot.requestId = id;
    m_snapshot.status = GTA_Stat_Request_Status::Queued; m_snapshot.requestedName = m_pending->statName;
    m_snapshot.detail = "Write queued for the GTA game thread"; ++m_snapshot.revision;
}

GTA_Stat_Snapshot GTA_Stats_State::Snapshot() const { std::scoped_lock lock(m_mutex); return m_snapshot; }
void GTA_Stats_State::Reset() { std::scoped_lock lock(m_mutex); m_pending.reset(); m_snapshot = {}; m_nextRequestId = 1; }
bool GTA_Stats_State::Consume(Command& command) { std::scoped_lock lock(m_mutex); if (!m_pending) return false; command = std::move(*m_pending); m_pending.reset(); return true; }
void GTA_Stats_State::Publish(GTA_Stat_Snapshot snapshot) { std::scoped_lock lock(m_mutex); if (snapshot.requestId < m_snapshot.requestId) return; snapshot.revision = m_snapshot.revision + 1; m_snapshot = std::move(snapshot); }

const char* GTAStatTypeName(GTA_Stat_Data::Type type) noexcept
{
    switch (type) {
    case GTA_Stat_Data::Type::Int: return "INT"; case GTA_Stat_Data::Type::Float: return "FLOAT"; case GTA_Stat_Data::Type::String: return "STRING";
    case GTA_Stat_Data::Type::Bool: return "BOOL"; case GTA_Stat_Data::Type::UInt8: return "UINT8"; case GTA_Stat_Data::Type::UInt16: return "UINT16";
    case GTA_Stat_Data::Type::UInt32: return "UINT32"; case GTA_Stat_Data::Type::UInt64: return "UINT64"; case GTA_Stat_Data::Type::Date: return "DATE";
    case GTA_Stat_Data::Type::Pos: return "POS"; case GTA_Stat_Data::Type::Int64: return "INT64"; default: return "UNKNOWN";
    }
}

const char* GTAStatRequestStatusName(GTA_Stat_Request_Status status) noexcept
{
    switch (status) {
    case GTA_Stat_Request_Status::Idle: return "Idle"; case GTA_Stat_Request_Status::Queued: return "Queued"; case GTA_Stat_Request_Status::Succeeded: return "Succeeded";
    case GTA_Stat_Request_Status::RuntimeUnavailable: return "Runtime unavailable"; case GTA_Stat_Request_Status::NotFound: return "Not found";
    case GTA_Stat_Request_Status::UnsupportedType: return "Unsupported type"; case GTA_Stat_Request_Status::ControlledByNetShop: return "Netshop controlled";
    case GTA_Stat_Request_Status::InvalidValue: return "Invalid value"; case GTA_Stat_Request_Status::Failed: return "Failed"; default: return "Unknown";
    }
}

void TickStatsExtension(GTA_Native_Manager& natives) noexcept
{
    GTA_Stats_State::Command command{}; auto& state = GTA_Stats_State::Instance(); if (!state.Consume(command)) return;
    try { state.Publish(Execute(natives, command)); } catch (...) {
        try { GTA_Stat_Snapshot result{}; result.requestId = command.id; result.requestedName = command.statName; result.status = GTA_Stat_Request_Status::Failed; result.detail = "Stat operation failed on the GTA game thread"; state.Publish(std::move(result)); } catch (...) {}
    }
}

void ResetStatsExtension() noexcept
{
    g_manager = nullptr; g_scanned = false;
    try { GTA_Stats_State::Instance().Reset(); } catch (...) {}
}
}
