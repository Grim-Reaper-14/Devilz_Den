#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace Devilz::Backend
{
class IExecutor;
}

namespace Devilz::Scripting::Lua
{
class Lua_Manager;

struct Lua_Self_Test_Result
{
    bool succeeded{};
    std::string message;
};

struct Lua_Runtime_Snapshot
{
    bool ready{};
    bool dedicatedThread{};
    std::size_t engines{};
    std::size_t scripts{};
    std::size_t modules{};
    std::size_t libraries{};
    std::size_t commands{};
    std::size_t scheduledTasks{};
    std::size_t pendingJobs{};
    std::string status{"Not started"};
};

using Lua_Runtime_Task = std::function<void(Lua_Manager&)>;

class Lua_Runtime final
{
public:
    static Lua_Runtime& Instance();

    ~Lua_Runtime();

    Lua_Runtime(const Lua_Runtime&) = delete;
    Lua_Runtime& operator=(const Lua_Runtime&) = delete;

    void Initialize();
    [[nodiscard]] bool Start(Backend::IExecutor& executor);
    [[nodiscard]] bool Submit(Lua_Runtime_Task task);
    void Shutdown() noexcept;

    [[nodiscard]] bool Ready() const;
    [[nodiscard]] Lua_Runtime_Snapshot Snapshot() const;
    [[nodiscard]] std::string Status() const;
    [[nodiscard]] std::string_view LuaVersion() const noexcept;
    [[nodiscard]] std::string_view Sol2Version() const noexcept;
    [[nodiscard]] Lua_Self_Test_Result RunSelfTest();

private:
    class Impl;

    Lua_Runtime();
    void ServiceLoop();

    std::unique_ptr<Impl> m_impl;
};
}
