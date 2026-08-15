#include "Lua_Scheduler.hpp"

#include "Lua_Engine.hpp"

#include <sol/sol.hpp>

#include <chrono>
#include <cstdint>
#include <optional>

namespace Devilz::Scripting::Lua
{
namespace
{
std::int64_t NowMilliseconds() noexcept
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

std::optional<sol::table> SchedulerTable(Lua_Engine& engine)
{
    auto& state = engine.State();
    const sol::object devilzObject = state["devilz"];
    if (!devilzObject.is<sol::table>())
        return std::nullopt;

    auto devilz = devilzObject.as<sol::table>();
    const sol::object schedulerObject = devilz["__scheduler"];
    if (!schedulerObject.is<sol::table>())
        return std::nullopt;

    return schedulerObject.as<sol::table>();
}
}

bool Lua_Scheduler::Install(Lua_Engine& engine)
{
    if (!engine.Ready())
        return false;

    auto result = engine.State().safe_script(R"lua(
        local scheduler = {
            tasks = {},
            next_id = 0
        }

        function scheduler.tick(now_ms)
            local index = 1
            while index <= #scheduler.tasks do
                local task = scheduler.tasks[index]
                if now_ms >= task.wake then
                    local ok, delay = coroutine.resume(task.co)
                    if not ok then
                        error(tostring(delay), 0)
                    end

                    if coroutine.status(task.co) == "dead" then
                        table.remove(scheduler.tasks, index)
                    else
                        local delay_ms = tonumber(delay) or 0
                        if delay_ms < 0 then
                            delay_ms = 0
                        end
                        task.wake = now_ms + delay_ms
                        index = index + 1
                    end
                else
                    index = index + 1
                end
            end
            return true
        end

        function devilz.create_thread(fn)
            if type(fn) ~= "function" then
                error("devilz.create_thread expects a function", 2)
            end

            scheduler.next_id = scheduler.next_id + 1
            table.insert(scheduler.tasks, {
                id = scheduler.next_id,
                co = coroutine.create(fn),
                wake = 0
            })
            return scheduler.next_id
        end

        devilz.async = devilz.create_thread

        function devilz.yield(milliseconds)
            local delay = tonumber(milliseconds) or 0
            if delay < 0 then
                delay = 0
            end
            return coroutine.yield(delay)
        end

        devilz.__scheduler = scheduler
        return true
    )lua", sol::script_pass_on_error);

    return result.valid();
}

Lua_Scheduler_Tick_Result Lua_Scheduler::Tick(Lua_Engine& engine)
{
    if (!engine.Ready())
        return {false, "Lua engine is not ready"};

    auto scheduler = SchedulerTable(engine);
    if (!scheduler)
        return {false, "Lua scheduler is not installed"};

    sol::protected_function tick = (*scheduler)["tick"];
    if (!tick.valid())
        return {false, "Lua scheduler tick function is unavailable"};

    auto result = tick(NowMilliseconds());
    if (!result.valid()) {
        const sol::error error = result;
        return {false, error.what()};
    }

    return {true, {}};
}

std::size_t Lua_Scheduler::TaskCount(Lua_Engine& engine)
{
    if (!engine.Ready())
        return 0;

    auto scheduler = SchedulerTable(engine);
    if (!scheduler)
        return 0;

    const sol::object tasksObject = (*scheduler)["tasks"];
    if (!tasksObject.is<sol::table>())
        return 0;

    return tasksObject.as<sol::table>().size();
}
}
