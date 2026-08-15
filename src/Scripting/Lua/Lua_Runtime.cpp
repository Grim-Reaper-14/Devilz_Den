#include "Lua_Runtime.hpp"

#include "Lua_Manager.hpp"

namespace Devilz::Scripting::Lua
{
class Lua_Runtime::Impl final
{
};

Lua_Runtime& Lua_Runtime::Instance()
{
    static Lua_Runtime runtime;
    return runtime;
}

Lua_Runtime::Lua_Runtime()
    : m_impl(std::make_unique<Impl>())
{
}

Lua_Runtime::~Lua_Runtime() = default;

void Lua_Runtime::Initialize()
{
    Lua_Manager::Instance().Initialize();
}

void Lua_Runtime::Shutdown() noexcept
{
    Lua_Manager::Instance().Shutdown();
}

bool Lua_Runtime::Ready() const noexcept
{
    return Lua_Manager::Instance().Ready();
}

std::string_view Lua_Runtime::Status() const noexcept
{
    return Lua_Manager::Instance().Status();
}

std::string_view Lua_Runtime::LuaVersion() const noexcept
{
    return Lua_Manager::Instance().LuaVersion();
}

std::string_view Lua_Runtime::Sol2Version() const noexcept
{
    return Lua_Manager::Instance().Sol2Version();
}

Lua_Self_Test_Result Lua_Runtime::RunSelfTest()
{
    const auto result = Lua_Manager::Instance().RunSelfTest();
    return {result.succeeded, result.message};
}
}
