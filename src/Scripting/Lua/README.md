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
    Settings/
      Lua_Settings_Binding.*
    Features/
      Lua_Features_Binding.*
    UI/
      Lua_UI_Binding.*
  Events/
    Lua_Event_Manager.*
  Settings/
    Lua_Setting_Manager.*
  Features/
    Lua_Feature_Manager.*
  UI/
    Lua_UI_Manager.*
  Fingerprint/
    Lua_Fingerprint.*
  HotReload/
    Lua_Hot_Reload_Manager.*
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

`Lua_Binding_Context` is the shared dependency surface passed to every binding library. It currently exposes script-owned commands, events, settings, features, UI descriptors/callbacks, the fingerprint manager, and an injected runtime logging callback. Future binding domains can receive controlled services through this context without expanding every binding-library method signature.

## Thread ownership

Production startup creates a dedicated `Lua` executor through the backend `ThreadManager`. The Lua service loop owns all Sol2 states, drains submitted jobs, scans script fingerprints for hot reload, dispatches events, processes Lua UI actions, and ticks script schedulers.

The renderer never touches a live `sol::state` or Lua callback. `Lua_UI_Manager` publishes plain descriptor snapshots into `Lua_Runtime_Snapshot`; ImGui renders those copied descriptors and queues only an element ID plus changed value back through `Lua_Runtime::Submit`. The callback then executes on the dedicated Lua service thread.

Each `.lua` script receives a separate `Lua_Engine`. Script-owned commands, event subscriptions, settings, features, and UI elements are removed before that engine is destroyed or rebuilt.

## Fingerprints

Lua fingerprints are deterministic 64-bit FNV-1a identifiers used for runtime compatibility, diagnostics, script change detection, and hot reload. They are not cryptographic signatures or authentication tokens.

The runtime fingerprint incorporates the Devilz Lua API version, precise Lua release, Sol2 version, binding compatibility versions (including UI), scheduler compatibility version, and hot-reload compatibility version. It remains stable across an identical restart and changes when one of those compatibility inputs changes.

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

## Hot reload

`Lua_Hot_Reload_Manager` polls loaded script fingerprints on the dedicated Lua service thread. The default interval is 250 ms, with a 50 ms minimum when changed programmatically. File timestamps are not used as the reload signal, so timestamp-only filesystem noise does not rebuild a script.

When content changes, the script keeps the same script ID/owner while its old owner resources and isolated engine are removed and rebuilt. This preserves owner identity for commands, events, settings, features, and UI across successful reloads without allowing stale callbacks or state to survive.

If a changed revision fails during Lua execution, any resources created by that failed revision are removed immediately, its engine is destroyed, and the script remains resident in `Error` with the failed content fingerprint. The watcher will not repeatedly execute the same broken bytes; after the file changes again, it retries the same script owner and can recover it back to `Running`.

If a script file temporarily cannot be read, the existing running engine is left alone. The watcher reports the fingerprint failure and retries on later scans.

Runtime snapshots expose whether hot reload is enabled plus scan, reload, and failure counts and the latest watcher status for the Lua menu page.

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

Event subscriptions are owner-scoped. One script cannot remove another script's subscription, and unloading or reloading a script removes all of its callbacks before its Lua state is destroyed.

```lua
local subscription = devilz.events.on(devilz.events.TICK, function()
    -- called from the dedicated Lua service thread
end)

local count = devilz.events.count()
devilz.events.off(subscription)
```

`tick` is the first built-in event. Additional controlled events can be added later for player, entity, vehicle, network, and other runtime domains.

## Settings binding

Settings are typed and owner-scoped. Supported values are Lua booleans, integers, floating-point numbers, and strings. A setting's type is fixed when it is registered; `set` rejects values of a different type instead of coercing them.

```lua
assert(devilz.settings.register("enabled", false))
assert(devilz.settings.register("attempts", 3))
assert(devilz.settings.register("scale", 1.25))
assert(devilz.settings.register("label", "example"))

print(devilz.settings.type("attempts")) -- integer
print(devilz.settings.get("scale"))

devilz.settings.set("enabled", true)
devilz.settings.reset("enabled")

print(devilz.settings.exists("label"))
print(devilz.settings.count())
devilz.settings.unregister("label")
```

Scripts with different owner IDs may use the same setting names without sharing values. Unloading or reloading a script removes all settings owned by that script before the replacement engine starts.

## Features binding

Features are controlled owner-scoped boolean toggles. The current registry is Lua-local infrastructure; it does not directly expose GTA memory, raw native state, or frontend internals. Future adapters can map approved Devilz features into this controlled layer.

```lua
assert(devilz.features.register("example_feature", false))

if devilz.features.available("example_feature") then
    devilz.features.set("example_feature", true)
end

print(devilz.features.enabled("example_feature"))

local ok, enabled = devilz.features.toggle("example_feature")
if ok then
    print(enabled)
end

devilz.features.reset("example_feature")
print(devilz.features.count())
devilz.features.unregister("example_feature")
```

Feature names are isolated by script owner and all entries are removed automatically when their owner script unloads or reloads.

## UI binding

Lua UI is declarative and owner-scoped. Scripts register controls on the Lua thread; the menu renders immutable snapshots. ImGui never invokes a Lua callback directly.

```lua
devilz.ui.section("My Script")
devilz.ui.text("Controls published from Lua")

local button = devilz.ui.button("Run Action", function()
    devilz.log.info("button pressed", "UI")
end)

local checkbox = devilz.ui.checkbox("Enabled", false, function(value)
    devilz.features.set("example_feature", value)
end)

local slider = devilz.ui.slider_float("Scale", 1.0, 0.0, 5.0, function(value)
    devilz.settings.set("scale", value)
end)

print(devilz.ui.count())
devilz.ui.remove(button)
```

Supported first-pass element types are section headings, wrapped text, buttons, checkboxes, and float sliders. Interactive callbacks are marshalled back to the Lua service thread with the changed value. Element IDs are owner-scoped for removal, and every element/callback is removed automatically when its script unloads or hot-reloads.

## Sandbox

Lua states currently open the base, coroutine, math, string, table, and UTF-8 libraries. Lua-side `dofile` and `loadfile` are disabled. Filesystem, OS, debug, package loading, raw memory access, and unrestricted native access are not exposed.

Future binding folders should follow the same domain layout, for example `Bindings/Players`, `Bindings/Entities`, `Bindings/Vehicles`, `Bindings/Weapons`, `Bindings/World`, and `Bindings/Config`.
