#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <sol/sol.hpp>

namespace Devilz::Scripting::Lua
{
class Lua_Engine final
{
public:
    using Id = std::uint64_t;

    explicit Lua_Engine(Id id, std::uint64_t ownerScriptId = 0);
    ~Lua_Engine();

    Lua_Engine(const Lua_Engine&) = delete;
    Lua_Engine& operator=(const Lua_Engine&) = delete;

    bool Initialize();
    void Shutdown() noexcept;

    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] Id GetId() const noexcept;
    [[nodiscard]] std::uint64_t OwnerScriptId() const noexcept;
    [[nodiscard]] std::string_view Status() const noexcept;

    sol::state& State();

private:
    Id m_id{};
    std::uint64_t m_ownerScriptId{};
    std::unique_ptr<sol::state> m_state;
    std::string m_status{"Not initialized"};
};
}
