# Lua Runtime

Devilz Den embeds Lua 5.4 through Sol2. Lua execution is owned by a dedicated backend executor, while each loaded script receives its own isolated Lua state and cooperative coroutine scheduler.

## Layout

```text
Scripting/Lua/
  Bindings/
    Lua_Binding_Context.hpp
    Core/
      Lua_Core_Binding.*
    Logger/
      Lua_Logger_Binding.*
    Events/
      Lua_Events_Binding.*
  Events/
    Lua_Event_Manager.*
  Lua_Manager.*
  Lua_Runtime.*
  Lua_Commands.*
  Lua_Engine.*
  Lua_Engine_Manager.*
  Lua_Script.*
  Lua_Script_Manager.*
  Lua_Module.*
  Lua_Module_Manager.*
  Lua_Scheduler.*
  Lua_Binding_Library.*
  Lua_Binding_Library_Manager.*
  Lua_Bindings.*              # compatibility facade for the original core API
```

`Lua_Binding_Context` is the shared dependency surface passed to every binding library. It currently exposes script-owned commands, the Lua event manager, and an injected runtime logging callback. Future binding domains can receive controlled services through this context without expanding every binding-library method signature.

## Thread ownership

Production startup creates a dedicated `Lua` executor through the backend `ThreadManager`. The Lua service loop owns all Sol2 states, drains submitted jobs, dispatches events, and ticks script schedulers. The renderer only reads immutable `Lua_Runtime_Snapshot` data and never touches a live `sol::state`.

Each `.lua` script receives a separate `Lua_Engine`. Script-owned commands and event subscriptions are removed before that engine is destroyed.

## Core API

```lua
devilz.api_version
devilz.runtime
devilz.engine_id
devilz.owner_script_id
devilz.version()

devilz.commands.register("name", function()
    -- command callback
end)

local ok, message = devilz.commands.execute("name")

devilz.create_thread(function()
    while true do
        devilz.yield(1000)
    end
end)

devilz.async(function()
    -- cooperative Lua task
end)
```

## Logger binding

The production runtime injects its already-running backend logger into Lua. A script never owns or starts a logger service.

```lua
devilz.log.info("script started")
devilz.log.warning("something looks wrong", "Inventory")
devilz.log.error("operation failed")
devilz.log.write("debug", "custom level call", "Diagnostics")

-- Alias for users who prefer the longer name.
devilz.logger.info("same logger")
```

Script log channels are automatically namespaced as `Lua.Script.<owner id>` with an optional child channel. The primary Lua state uses the `Lua` service name. `devilz.log.available` reports whether a runtime logger was injected.

## Events binding

Event subscriptions are owner-scoped. One script cannot remove another script's subscription, and unloading a script removes all of its callbacks before its Lua state is destroyed.

```lua
local subscription = devilz.events.on(devilz.events.TICK, function()
    -- called from the dedicated Lua service thread
end)

local count = devilz.events.count()
devilz.events.off(subscription)
```

`tick` is the first built-in event. Additional controlled events can be added later for UI, player, entity, vehicle, network, and other runtime domains.

## Sandbox

Lua states currently open the base, coroutine, math, string, table, and UTF-8 libraries. Lua-side `dofile` and `loadfile` are disabled. Filesystem, OS, debug, package loading, raw memory access, and unrestricted native access are not exposed.

Future binding folders should follow the same domain layout, for example `Bindings/Settings`, `Bindings/UI`, `Bindings/Players`, `Bindings/Entities`, `Bindings/Vehicles`, `Bindings/Weapons`, `Bindings/World`, and `Bindings/Config`.
