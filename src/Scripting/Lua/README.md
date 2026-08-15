# Lua Runtime

Devilz Den embeds Lua 5.4 through Sol2 and separates runtime ownership from scripts, modules, commands, engines, binding libraries, and scheduling.

```text
Scripting/Lua/
  Lua_Manager.*
  Lua_Runtime.*
  Lua_Scheduler.*
  Lua_Commands.*
  Lua_Engine.*
  Lua_Engine_Manager.*
  Lua_Script.*
  Lua_Script_Manager.*
  Lua_Module.*
  Lua_Module_Manager.*
  Lua_Bindings.*
  Lua_Binding_Library.*
  Lua_Binding_Library_Manager.*
```

## Thread ownership

Production Lua execution is started by `Runtime_Manager` on a dedicated single-worker executor created through `ThreadManager::CreateDedicated("Lua")`. The long-lived Lua service loop owns all Sol2/Lua state access. Renderer/UI code reads a copied `Lua_Runtime_Snapshot` and queues work through `Lua_Runtime` instead of touching live Lua containers.

Shutdown runs in the opposite direction: the Lua service is stopped and its engines are destroyed before the backend thread manager is stopped. The old synchronous `Lua_Runtime::Initialize()` path remains available for isolated compatibility/testing, but normal runtime ownership is threaded.

## Script isolation and cooperative tasks

`Lua_Engine_Manager` creates isolated Sol2 states, including one engine per loaded script. A script therefore has its own globals and coroutine scheduler while all script engines are executed by the same dedicated Lua backend thread.

The core binding exposes cooperative tasks:

```lua
local task = devilz.create_thread(function()
    while true do
        -- Lua work
        devilz.yield(1000)
    end
end)

-- Alias for create_thread.
devilz.async(function()
    devilz.yield(100)
end)
```

`devilz.yield(milliseconds)` yields the current Lua coroutine. The C++ scheduler resumes due coroutines from the dedicated Lua thread. These are cooperative Lua tasks, not one operating-system thread per script.

## Current binding surface

The core binding library exposes `devilz.api_version`, runtime/engine metadata, `devilz.version()`, the script-owned `devilz.commands` API, `devilz.create_thread`, `devilz.async`, and `devilz.yield`.

Lua states open the base, coroutine, math, string, table, and UTF-8 libraries. Lua-side `dofile` and `loadfile` remain disabled; filesystem, OS, debug, and unrestricted native access are not exposed.

Future game-sensitive bindings should queue work to the validated GTA/game-thread bridge rather than calling game-thread-only functionality directly from the Lua executor.
