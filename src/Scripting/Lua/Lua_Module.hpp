#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace Devilz::Scripting::Lua
{
class Lua_Module final
{
public:
    Lua_Module(std::string name, std::filesystem::path path);

    [[nodiscard]] std::string_view Name() const noexcept;
    [[nodiscard]] const std::filesystem::path& Path() const noexcept;

private:
    std::string m_name;
    std::filesystem::path m_path;
};
}
