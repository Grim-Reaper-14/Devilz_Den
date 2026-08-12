#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace Devilz::Scripting::Lua
{
struct Lua_Self_Test_Result
{
    bool succeeded{};
    std::string message;
};

class Lua_Runtime final
{
public:
    static Lua_Runtime& Instance();

    ~Lua_Runtime();

    Lua_Runtime(const Lua_Runtime&) = delete;
    Lua_Runtime& operator=(const Lua_Runtime&) = delete;

    void Initialize();
    void Shutdown() noexcept;

    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] std::string_view Status() const noexcept;
    [[nodiscard]] std::string_view LuaVersion() const noexcept;
    [[nodiscard]] std::string_view Sol2Version() const noexcept;
    [[nodiscard]] Lua_Self_Test_Result RunSelfTest();

private:
    class Impl;

    Lua_Runtime();

    std::unique_ptr<Impl> m_impl;
};
}
