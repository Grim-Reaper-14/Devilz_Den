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
  Fingerprint/
    Lua_Fingerprint.*
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

`Lua_Binding_Context` is the shared dependency surface passed to every binding library. It currently exposes script-owned commands, the Lua event manager, the fingerprint manager, and an injected runtime logging callback. Future binding domains can receive controlled services through this context without expanding every binding-library method signature.

## Thread ownership

Production startup creates a dedicated `Lua` executor through the backend `ThreadManager`. The Lua service loop owns all Sol2 states, drains submitted jobs, dispatches events, and ticks script schedulers. The renderer only reads immutable `Lua_Runtime_Snapshot` data and never touches a live `sol::state`.

Each `.lua` script receives a separate `Lua_Engine`. Script-owned commands and event subscriptions are removed before that engine is destroyed.

## Fingerprints

Lua fingerprints are deterministic 64-bit FNV-1a identifiers used for runtime compatibility, diagnostics, and script change detection. They are not cryptographic signatures or authentication tokens.

The runtime fingerprint incorporates the Devilz Lua API version, precise Lua release, Sol2 version, and binding/scheduler compatibility versions. It remains stable across an identical restart and changes when one of those compatibility inputs changes.

```lua
print(devilz.fingerprint)                  -- e.g. 0x0123456789ABCDEF
print(devilz.runtime_fingerprint)          -- same runtime fingerprint
print(devilz.runtime_info.fingerprint)
print(devilz.runtime_info.api_version)
print(devilz.runtime_info.lua_version)
print(devilz.runtime_info.sol2_version)
```

Every loaded script receives a content-based fingerprint before the script executes:

```lua
print(devilz.script.id)
print(devilz.script.path)
print(devilz.script.fingerprint)
print(devilz.script.content_hash)
print(devilz.script.runtime_fingerprint)
```

The script path is intentionally excluded from the fingerprint. Renaming or moving an unchanged script preserves its content hash and combined script fingerprint. Changing the file bytes changes both the content hash and combined fingerprint. The combined script fingerprint includes the active runtime fingerprint, so the same script content can also be distinguished across incompatible Devilz Lua API builds.

Fingerprints are exposed to Lua as fixed-width hexadecimal strings so all 64 bits remain unambiguous in the Lua API. C++ retains the raw `std::uint64_t` values through `Lua_Fingerprint_Manager`, `Lua_Script`, and `Lua_Runtime_Snapshot`.

## Core API

```lua
devilz.api_version
devilz.runtime
devilz.engine_id
devilz.owner_script_id
devilz.fingerprint
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
