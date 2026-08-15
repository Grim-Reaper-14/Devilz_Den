#pragma once

#include "Lua_Engine.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace Devilz::Scripting::Lua
{
class Lua_Engine_Manager final
{
public:
    Lua_Engine& CreateEngine(std::uint64_t ownerScriptId = 0);
    bool DestroyEngine(Lua_Engine::Id id) noexcept;

    [[nodiscard]] Lua_Engine* FindEngine(Lua_Engine::Id id) noexcept;
    [[nodiscard]] const Lua_Engine* FindEngine(Lua_Engine::Id id) const noexcept;
    [[nodiscard]] std::size_t Count() const noexcept;

    void Clear() noexcept;

private:
    Lua_Engine::Id m_nextId{1};
    std::unordered_map<Lua_Engine::Id, std::unique_ptr<Lua_Engine>> m_engines;
};
}
