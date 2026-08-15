#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Devilz::Scripting::Lua
{
class Lua_Setting_Manager;

struct Lua_Config_Result
{
    bool succeeded{};
    std::size_t applied{};
    std::string message;
};

class Lua_Config_Manager final
{
public:
    using Owner = std::uint64_t;

    bool AttachOwner(Owner owner, const std::filesystem::path& scriptPath);
    void DetachOwner(Owner owner) noexcept;
    void ClearOwners() noexcept;

    [[nodiscard]] bool Available(Owner owner) const noexcept;
    [[nodiscard]] std::string ScriptKey(Owner owner) const;

    [[nodiscard]] Lua_Config_Result Save(
        Owner owner,
        std::string_view profile,
        const Lua_Setting_Manager& settings) const;
    [[nodiscard]] Lua_Config_Result Load(
        Owner owner,
        std::string_view profile,
        Lua_Setting_Manager& settings) const;
    [[nodiscard]] Lua_Config_Result Remove(Owner owner, std::string_view profile) const;
    [[nodiscard]] std::vector<std::string> List(Owner owner) const;

    [[nodiscard]] static std::filesystem::path RootDirectory();

private:
    [[nodiscard]] static bool ValidProfile(std::string_view profile) noexcept;
    [[nodiscard]] static std::string MakeScriptKey(const std::filesystem::path& path);
    [[nodiscard]] std::filesystem::path OwnerDirectory(Owner owner) const;
    [[nodiscard]] std::filesystem::path ProfilePath(Owner owner, std::string_view profile) const;

    std::unordered_map<Owner, std::string> m_ownerKeys;
};
}
