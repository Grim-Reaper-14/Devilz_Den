#include "Devils_Aimbot.hpp"

#include "Backend/Logging/LoggerService.hpp"
#include "Backend/Memory/Module_Manager.hpp"
#include "Backend/Memory/Pattern.hpp"
#include "Backend/Memory/Pattern_Scanner.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::uint64_t SupportedFingerprint = 0x6A4F97F605B81000ULL;
constexpr std::string_view LogService = "GTA5_Enhanced.Weapons.Devils_Aimbot";

struct Patch_Site
{
    static constexpr std::size_t Capacity = 8;

    std::string name;
    std::uintptr_t address = 0;
    std::array<std::byte, Capacity> original{};
    std::array<std::byte, Capacity> replacement{};
    std::size_t size = 0;
    bool applied = false;

    [[nodiscard]] bool Ready() const noexcept
    {
        return address != 0 && size != 0 && size <= Capacity;
    }
};

bool IsExecutableProtection(DWORD protection) noexcept
{
    const DWORD base = protection & 0xFFU;
    return base == PAGE_EXECUTE || base == PAGE_EXECUTE_READ ||
           base == PAGE_EXECUTE_READWRITE || base == PAGE_EXECUTE_WRITECOPY;
}

bool IsExecutableRange(std::uintptr_t address, std::size_t size) noexcept
{
    if (address == 0 || size == 0)
        return false;

    MEMORY_BASIC_INFORMATION info{};
    if (::VirtualQuery(reinterpret_cast<const void*>(address), &info, sizeof(info)) != sizeof(info))
        return false;
    if (info.State != MEM_COMMIT || (info.Protect & PAGE_GUARD) != 0 ||
        (info.Protect & PAGE_NOACCESS) != 0 || !IsExecutableProtection(info.Protect))
        return false;

    const auto regionBegin = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
    const auto regionEnd = regionBegin + info.RegionSize;
    return address >= regionBegin && size <= info.RegionSize && address <= regionEnd - size;
}

bool WritePatch(Patch_Site& patch, bool apply) noexcept
{
    if (!patch.Ready() || !IsExecutableRange(patch.address, patch.size))
        return false;

    const auto& expected = apply ? patch.original : patch.replacement;
    const auto& desired = apply ? patch.replacement : patch.original;

    std::array<std::byte, Patch_Site::Capacity> current{};
    std::memcpy(current.data(), reinterpret_cast<const void*>(patch.address), patch.size);
    if (std::equal(current.begin(), current.begin() + patch.size, desired.begin())) {
        patch.applied = apply;
        return true;
    }
    if (!std::equal(current.begin(), current.begin() + patch.size, expected.begin()))
        return false;

    DWORD oldProtection = 0;
    if (::VirtualProtect(reinterpret_cast<void*>(patch.address), patch.size, PAGE_EXECUTE_READWRITE, &oldProtection) == FALSE)
        return false;

    std::memcpy(reinterpret_cast<void*>(patch.address), desired.data(), patch.size);
    ::FlushInstructionCache(::GetCurrentProcess(), reinterpret_cast<const void*>(patch.address), patch.size);

    DWORD ignored = 0;
    const bool restoredProtection = ::VirtualProtect(
        reinterpret_cast<void*>(patch.address), patch.size, oldProtection, &ignored) != FALSE;

    std::array<std::byte, Patch_Site::Capacity> verify{};
    std::memcpy(verify.data(), reinterpret_cast<const void*>(patch.address), patch.size);
    if (!std::equal(verify.begin(), verify.begin() + patch.size, desired.begin()) || !restoredProtection)
        return false;

    patch.applied = apply;
    return true;
}

bool RestorePatch(Patch_Site& patch) noexcept
{
    if (!patch.Ready())
        return true;

    std::array<std::byte, Patch_Site::Capacity> current{};
    std::memcpy(current.data(), reinterpret_cast<const void*>(patch.address), patch.size);
    if (std::equal(current.begin(), current.begin() + patch.size, patch.original.begin())) {
        patch.applied = false;
        return true;
    }
    if (!std::equal(current.begin(), current.begin() + patch.size, patch.replacement.begin()))
        return false;

    return WritePatch(patch, false);
}

bool AddressInExecutableSection(
    const Backend::Module_Info& module,
    std::uintptr_t address,
    std::size_t size) noexcept
{
    return std::any_of(module.sections.begin(), module.sections.end(), [&](const auto& section) {
        return section.Executable() && section.range.Contains(address, size);
    });
}

bool ResolvePatch(
    const Backend::Module_Info& module,
    std::string_view name,
    std::string_view signature,
    std::ptrdiff_t patchOffset,
    std::initializer_list<std::uint8_t> replacement,
    Patch_Site& out,
    std::string& error)
{
    auto compiled = Backend::Pattern::Compile(signature);
    if (!compiled) {
        error = std::string(name) + ": pattern compile failed";
        return false;
    }

    Backend::Pattern_Scanner scanner;
    std::vector<std::uintptr_t> matches;
    for (const auto& section : module.sections) {
        if (!section.Executable())
            continue;

        Backend::Pattern_Scanner::Options options{};
        options.maxResults = 2;
        options.collectStatistics = false;
        auto result = scanner.Scan(section.range, compiled.Value(), options);
        if (!result)
            continue;

        for (const auto& match : result.Value()) {
            matches.push_back(match.address.Address());
            if (matches.size() > 1)
                break;
        }
        if (matches.size() > 1)
            break;
    }

    if (matches.empty()) {
        error = std::string(name) + ": pattern not found";
        return false;
    }
    if (matches.size() != 1) {
        error = std::string(name) + ": pattern was not unique";
        return false;
    }

    const auto target = static_cast<std::uintptr_t>(
        static_cast<std::intptr_t>(matches.front()) + patchOffset);
    if (replacement.size() == 0 || replacement.size() > Patch_Site::Capacity ||
        !AddressInExecutableSection(module, target, replacement.size()) ||
        !IsExecutableRange(target, replacement.size())) {
        error = std::string(name) + ": patch target is outside executable GTA code";
        return false;
    }

    out.name = std::string(name);
    out.address = target;
    out.size = replacement.size();
    std::size_t index = 0;
    for (const auto value : replacement)
        out.replacement[index++] = static_cast<std::byte>(value);
    std::memcpy(out.original.data(), reinterpret_cast<const void*>(target), out.size);

    if (std::equal(out.original.begin(), out.original.begin() + out.size, out.replacement.begin())) {
        error = std::string(name) + ": target already contains patched bytes";
        out = {};
        return false;
    }

    return true;
}

class Devils_Aimbot_Service
{
public:
    bool Configure(Backend::LoggerService& logger, std::uint64_t fingerprint)
    {
        std::scoped_lock lock(m_mutex);
        RestoreAllLocked();
        ClearLocked();
        m_logger = &logger;

        if (fingerprint != SupportedFingerprint) {
            m_status = Devils_Aimbot_Status::UnsupportedBuild;
            m_detail = "Unsupported GTA Enhanced build fingerprint";
            LogLocked(Backend::LogLevel::Warning, m_detail);
            return false;
        }

        Backend::Module_Manager modules;
        auto module = modules.FindLoaded("GTA5_Enhanced.exe");
        if (!module)
            module = modules.MainModule();
        if (!module) {
            m_status = Devils_Aimbot_Status::PatternFailure;
            m_detail = "GTA5_Enhanced.exe module image is unavailable";
            LogLocked(Backend::LogLevel::Warning, m_detail);
            return false;
        }

        std::string error;
        if (!ResolvePatch(module.Value(), "ShouldNotTargetEntity",
                "F6 80 A9 14 00 00 01", -0x53,
                {0xB0, 0x00, 0xC3}, m_shouldNotTarget, error) ||
            !ResolvePatch(module.Value(), "GetAssistedAimType",
                "FF E0 48 8D 86", -0x15,
                {0xBD, 0x01, 0x00, 0x00, 0x00}, m_assistedAimType, error) ||
            !ResolvePatch(module.Value(), "GetLockOnPos",
                "0F 29 74 24 ? 48 89 D6 48 89 CF 48 8B 05", 0x22,
                {0xEB}, m_lockOnHead, error) ||
            !ResolvePatch(module.Value(), "ShouldAllowDriverLockOn",
                "75 ? 45 89 C7 49 89 CE", -0x2C,
                {0xB0, 0x01, 0xC3}, m_targetDrivers, error)) {
            m_status = Devils_Aimbot_Status::PatternFailure;
            m_detail = error;
            ClearPatchSitesLocked();
            LogLocked(Backend::LogLevel::Warning, "Unavailable | " + m_detail);
            return false;
        }

        m_status = Devils_Aimbot_Status::Ready;
        m_detail = "Ready | Devils_Aimbot assisted-aim patches resolved";
        LogLocked(Backend::LogLevel::Info, m_detail);
        return true;
    }

    void Reset()
    {
        std::scoped_lock lock(m_mutex);
        RestoreAllLocked();
        ClearLocked();
    }

    [[nodiscard]] Devils_Aimbot_Status Status() const noexcept
    {
        std::scoped_lock lock(m_mutex);
        return m_status;
    }

    [[nodiscard]] bool Available() const noexcept
    {
        std::scoped_lock lock(m_mutex);
        return m_status == Devils_Aimbot_Status::Ready;
    }

    [[nodiscard]] bool Enabled() const noexcept
    {
        std::scoped_lock lock(m_mutex);
        return m_enabled;
    }

    [[nodiscard]] bool AimForHead() const noexcept
    {
        std::scoped_lock lock(m_mutex);
        return m_aimForHead;
    }

    [[nodiscard]] bool TargetDrivers() const noexcept
    {
        std::scoped_lock lock(m_mutex);
        return m_targetDriversRequested;
    }

    bool SetEnabled(bool enabled)
    {
        std::scoped_lock lock(m_mutex);
        if (m_status != Devils_Aimbot_Status::Ready)
            return false;
        if (m_enabled == enabled)
            return true;

        if (enabled) {
            if (!WritePatch(m_shouldNotTarget, true))
                return PatchFailureLocked("Failed to apply ShouldNotTargetEntity patch");
            if (!WritePatch(m_assistedAimType, true)) {
                RestorePatch(m_shouldNotTarget);
                return PatchFailureLocked("Failed to apply GetAssistedAimType patch");
            }
            if (m_aimForHead && !WritePatch(m_lockOnHead, true)) {
                RestorePatch(m_assistedAimType);
                RestorePatch(m_shouldNotTarget);
                return PatchFailureLocked("Failed to apply Aim For Head patch");
            }
            if (m_targetDriversRequested && !WritePatch(m_targetDrivers, true)) {
                RestorePatch(m_lockOnHead);
                RestorePatch(m_assistedAimType);
                RestorePatch(m_shouldNotTarget);
                return PatchFailureLocked("Failed to apply Target Drivers patch");
            }
            m_enabled = true;
            LogLocked(Backend::LogLevel::Info, "Devils_Aimbot enabled");
            return true;
        }

        const bool restoredDrivers = RestorePatch(m_targetDrivers);
        const bool restoredHead = RestorePatch(m_lockOnHead);
        const bool restoredAimType = RestorePatch(m_assistedAimType);
        const bool restoredTargeting = RestorePatch(m_shouldNotTarget);
        if (!(restoredDrivers && restoredHead && restoredAimType && restoredTargeting))
            return PatchFailureLocked("Failed to restore one or more aimbot patches");

        m_enabled = false;
        LogLocked(Backend::LogLevel::Info, "Devils_Aimbot disabled and original bytes restored");
        return true;
    }

    bool SetAimForHead(bool enabled)
    {
        std::scoped_lock lock(m_mutex);
        if (m_status != Devils_Aimbot_Status::Ready)
            return false;
        if (m_aimForHead == enabled)
            return true;

        if (m_enabled && !(enabled ? WritePatch(m_lockOnHead, true) : RestorePatch(m_lockOnHead)))
            return PatchFailureLocked(enabled ? "Failed to apply Aim For Head patch" : "Failed to restore Aim For Head patch");

        m_aimForHead = enabled;
        LogLocked(Backend::LogLevel::Info, std::string("Aim For Head ") + (enabled ? "enabled" : "disabled"));
        return true;
    }

    bool SetTargetDrivers(bool enabled)
    {
        std::scoped_lock lock(m_mutex);
        if (m_status != Devils_Aimbot_Status::Ready)
            return false;
        if (m_targetDriversRequested == enabled)
            return true;

        if (m_enabled && !(enabled ? WritePatch(m_targetDrivers, true) : RestorePatch(m_targetDrivers)))
            return PatchFailureLocked(enabled ? "Failed to apply Target Drivers patch" : "Failed to restore Target Drivers patch");

        m_targetDriversRequested = enabled;
        LogLocked(Backend::LogLevel::Info, std::string("Target Drivers ") + (enabled ? "enabled" : "disabled"));
        return true;
    }

private:
    void LogLocked(Backend::LogLevel level, std::string message) const noexcept
    {
        if (!m_logger)
            return;
        try {
            m_logger->Log(level, std::move(message), std::string(LogService));
        } catch (...) {
        }
    }

    bool PatchFailureLocked(std::string detail) noexcept
    {
        RestoreAllLocked();
        m_status = Devils_Aimbot_Status::PatchFailure;
        m_detail = std::move(detail);
        LogLocked(Backend::LogLevel::Error, m_detail);
        return false;
    }

    void RestoreAllLocked() noexcept
    {
        RestorePatch(m_targetDrivers);
        RestorePatch(m_lockOnHead);
        RestorePatch(m_assistedAimType);
        RestorePatch(m_shouldNotTarget);
        m_enabled = false;
    }

    void ClearPatchSitesLocked() noexcept
    {
        m_shouldNotTarget = {};
        m_assistedAimType = {};
        m_lockOnHead = {};
        m_targetDrivers = {};
    }

    void ClearLocked() noexcept
    {
        ClearPatchSitesLocked();
        m_status = Devils_Aimbot_Status::Uninitialized;
        m_detail = "Devils_Aimbot is not initialized";
        m_enabled = false;
        m_aimForHead = false;
        m_targetDriversRequested = false;
        m_logger = nullptr;
    }

    mutable std::mutex m_mutex;
    Backend::LoggerService* m_logger = nullptr;
    Devils_Aimbot_Status m_status = Devils_Aimbot_Status::Uninitialized;
    std::string m_detail = "Devils_Aimbot is not initialized";
    Patch_Site m_shouldNotTarget;
    Patch_Site m_assistedAimType;
    Patch_Site m_lockOnHead;
    Patch_Site m_targetDrivers;
    bool m_enabled = false;
    bool m_aimForHead = false;
    bool m_targetDriversRequested = false;
};

Devils_Aimbot_Service& Service()
{
    static Devils_Aimbot_Service service;
    return service;
}
}

bool ConfigureDevilsAimbot(Backend::LoggerService& logger, std::uint64_t buildFingerprint)
{
    return Service().Configure(logger, buildFingerprint);
}

void ResetDevilsAimbot()
{
    Service().Reset();
}

Devils_Aimbot_Status DevilsAimbotStatus() noexcept
{
    return Service().Status();
}

const char* DevilsAimbotStatusText() noexcept
{
    switch (Service().Status()) {
    case Devils_Aimbot_Status::Uninitialized: return "Not initialized";
    case Devils_Aimbot_Status::Ready: return "Ready";
    case Devils_Aimbot_Status::UnsupportedBuild: return "Unsupported GTA build";
    case Devils_Aimbot_Status::PatternFailure: return "Pattern scan failed - see log";
    case Devils_Aimbot_Status::PatchFailure: return "Patch failed - see log";
    default: return "Unknown";
    }
}

bool DevilsAimbotAvailable() noexcept
{
    return Service().Available();
}

bool DevilsAimbotEnabled() noexcept
{
    return Service().Enabled();
}

bool DevilsAimbotAimForHead() noexcept
{
    return Service().AimForHead();
}

bool DevilsAimbotTargetDrivers() noexcept
{
    return Service().TargetDrivers();
}

bool SetDevilsAimbotEnabled(bool enabled)
{
    return Service().SetEnabled(enabled);
}

bool SetDevilsAimbotAimForHead(bool enabled)
{
    return Service().SetAimForHead(enabled);
}

bool SetDevilsAimbotTargetDrivers(bool enabled)
{
    return Service().SetTargetDrivers(enabled);
}
}
