#pragma once

#include "Lua_Binding_Library.hpp"

#include <memory>
#include <string_view>
#include <vector>

namespace Devilz::Scripting::Lua
{
class Lua_Binding_Context;
class Lua_Engine;

class Lua_Binding_Library_Manager final
{
public:
    bool RegisterLibrary(std::unique_ptr<Lua_Binding_Library> library);
    bool BindAll(Lua_Engine& engine, const Lua_Binding_Context& context);

    [[nodiscard]] Lua_Binding_Library* FindLibrary(std::string_view name) noexcept;
    [[nodiscard]] const Lua_Binding_Library* FindLibrary(std::string_view name) const noexcept;
    [[nodiscard]] std::size_t Count() const noexcept;

    void Clear() noexcept;

private:
    std::vector<std::unique_ptr<Lua_Binding_Library>> m_libraries;
};
}
