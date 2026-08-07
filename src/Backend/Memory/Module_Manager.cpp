#include "Module_Manager.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>

#include <algorithm>
#include <array>
#include <cstring>

namespace Devilz::Backend
{
namespace
{
Error ModuleError(std::string message, std::string detail = {})
{
    auto error = Error(ErrorCode::RuntimeFailure, ErrorCategory::Platform, std::move(message));
    if (!detail.empty()) error.With("Module", std::move(detail));
    return error;
}

std::string Narrow(const wchar_t* value)
{
    if (!value || !*value) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (count <= 1) return {};
    std::string out(static_cast<std::size_t>(count - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, out.data(), count, nullptr, nullptr);
    return out;
}
}

bool Module_Section::Readable() const noexcept { return (characteristics & IMAGE_SCN_MEM_READ) != 0; }
bool Module_Section::Writable() const noexcept { return (characteristics & IMAGE_SCN_MEM_WRITE) != 0; }
bool Module_Section::Executable() const noexcept { return (characteristics & IMAGE_SCN_MEM_EXECUTE) != 0; }

Result<Module_Info> Module_Manager::MainModule() const
{
    return Inspect(reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr)));
}

Result<Module_Info> Module_Manager::FindLoaded(std::string_view moduleName) const
{
    if (moduleName.empty()) return MainModule();

    const int wideCount = MultiByteToWideChar(CP_UTF8, 0, moduleName.data(), static_cast<int>(moduleName.size()), nullptr, 0);
    if (wideCount <= 0)
        return Result<Module_Info>::Failure(ModuleError("Unable to convert module name", std::string(moduleName)));

    std::wstring wide(static_cast<std::size_t>(wideCount), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, moduleName.data(), static_cast<int>(moduleName.size()), wide.data(), wideCount);
    HMODULE module = GetModuleHandleW(wide.c_str());
    if (!module)
        return Result<Module_Info>::Failure(Error::FromWin32(ErrorCode::RuntimeFailure, ErrorCategory::Platform,
            GetLastError(), "Loaded module was not found").With("Module", std::string(moduleName)));
    return Inspect(reinterpret_cast<std::uintptr_t>(module));
}

Result<Module_Info> Module_Manager::Inspect(std::uintptr_t moduleBase) const
{
    if (!moduleBase)
        return Result<Module_Info>::Failure(ModuleError("Module base is null"));

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(moduleBase);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return Result<Module_Info>::Failure(ModuleError("Invalid DOS signature"));

    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(moduleBase + static_cast<std::uintptr_t>(dos->e_lfanew));
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return Result<Module_Info>::Failure(ModuleError("Invalid or unsupported PE image"));

    Module_Info info;
    info.base = moduleBase;
    info.size = nt->OptionalHeader.SizeOfImage;
    info.fingerprint.timeDateStamp = nt->FileHeader.TimeDateStamp;
    info.fingerprint.sizeOfImage = nt->OptionalHeader.SizeOfImage;

    wchar_t path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(reinterpret_cast<HMODULE>(moduleBase), path, MAX_PATH);
    if (length) {
        info.path = std::filesystem::path(path);
        info.name = Narrow(info.path.filename().c_str());
    }

    const auto* section = IMAGE_FIRST_SECTION(nt);
    info.sections.reserve(nt->FileHeader.NumberOfSections);
    for (std::uint16_t i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        char name[IMAGE_SIZEOF_SHORT_NAME + 1]{};
        std::memcpy(name, section[i].Name, IMAGE_SIZEOF_SHORT_NAME);
        const std::size_t virtualSize = std::max<std::size_t>(section[i].Misc.VirtualSize, section[i].SizeOfRawData);
        if (section[i].VirtualAddress >= info.size) continue;
        const std::size_t boundedSize = std::min<std::size_t>(virtualSize, info.size - section[i].VirtualAddress);
        info.sections.push_back({name, {moduleBase + section[i].VirtualAddress, boundedSize}, section[i].Characteristics});
    }

    info.fingerprint.imageHash = HashImageMetadata(info);
    return Result<Module_Info>::Success(std::move(info));
}

Result<Memory_Range> Module_Manager::FindSection(const Module_Info& module, std::string_view sectionName) const
{
    const auto it = std::find_if(module.sections.begin(), module.sections.end(), [&](const Module_Section& section) {
        return section.name == sectionName;
    });
    if (it == module.sections.end())
        return Result<Memory_Range>::Failure(ModuleError("PE section not found", module.name).With("Section", std::string(sectionName)));
    return Result<Memory_Range>::Success(it->range);
}

std::vector<Memory_Range> Module_Manager::SelectSections(const Module_Info& module, Module_Section_Access access) const
{
    std::vector<Memory_Range> ranges;
    for (const auto& section : module.sections) {
        bool selected = false;
        switch (access) {
        case Module_Section_Access::Any: selected = true; break;
        case Module_Section_Access::Readable: selected = section.Readable(); break;
        case Module_Section_Access::Writable: selected = section.Writable(); break;
        case Module_Section_Access::Executable: selected = section.Executable(); break;
        case Module_Section_Access::ExecutableReadable: selected = section.Executable() && section.Readable(); break;
        }
        if (selected && !section.range.Empty()) ranges.push_back(section.range);
    }
    return ranges;
}

std::uint64_t Module_Manager::HashImageMetadata(const Module_Info& module) noexcept
{
    std::uint64_t hash = 1469598103934665603ull;
    const auto mix = [&](std::uint64_t value) mutable {
        for (int i = 0; i < 8; ++i) { hash ^= static_cast<std::uint8_t>(value); hash *= 1099511628211ull; value >>= 8; }
    };
    mix(module.fingerprint.timeDateStamp);
    mix(module.fingerprint.sizeOfImage);
    for (const auto& section : module.sections) {
        for (const unsigned char c : section.name) { hash ^= c; hash *= 1099511628211ull; }
        mix(section.range.size);
        mix(section.characteristics);
    }
    return hash;
}
}
