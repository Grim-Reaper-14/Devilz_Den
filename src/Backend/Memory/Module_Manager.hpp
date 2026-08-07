#pragma once

#include "Backend/Error/Result.hpp"
#include "Memory_Range.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Devilz::Backend
{
enum class Module_Section_Access : std::uint8_t
{
    Any,
    Readable,
    Writable,
    Executable,
    ExecutableReadable
};

struct Module_Section
{
    std::string name;
    Memory_Range range;
    std::uint32_t characteristics = 0;

    [[nodiscard]] bool Readable() const noexcept;
    [[nodiscard]] bool Writable() const noexcept;
    [[nodiscard]] bool Executable() const noexcept;
};

struct Module_Fingerprint
{
    std::uint32_t timeDateStamp = 0;
    std::uint32_t sizeOfImage = 0;
    std::uint64_t imageHash = 0;

    [[nodiscard]] bool operator==(const Module_Fingerprint&) const noexcept = default;
};

struct Module_Info
{
    std::string name;
    std::filesystem::path path;
    std::uintptr_t base = 0;
    std::size_t size = 0;
    Module_Fingerprint fingerprint;
    std::vector<Module_Section> sections;

    [[nodiscard]] Memory_Range ImageRange() const noexcept { return {base, size}; }
};

class Module_Manager final
{
public:
    [[nodiscard]] Result<Module_Info> MainModule() const;
    [[nodiscard]] Result<Module_Info> FindLoaded(std::string_view moduleName) const;
    [[nodiscard]] Result<Module_Info> Inspect(std::uintptr_t moduleBase) const;

    [[nodiscard]] Result<Memory_Range> FindSection(
        const Module_Info& module,
        std::string_view sectionName) const;

    [[nodiscard]] std::vector<Memory_Range> SelectSections(
        const Module_Info& module,
        Module_Section_Access access) const;

private:
    static std::uint64_t HashImageMetadata(const Module_Info& module) noexcept;
};
}
