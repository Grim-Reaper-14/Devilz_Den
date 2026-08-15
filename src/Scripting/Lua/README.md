# Lua Runtime

Devilz Den embeds Lua 5.4 through Sol2 and now separates runtime ownership from scripts, modules, commands, engines, and binding libraries.

```text
Scripting/Lua/
  Lua_Manager.*
  Lua_Runtime.*                 # compatibility facade used by the existing UI/test
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

`Lua_Manager` owns the subsystem lifecycle. `Lua_Engine_Manager` creates isolated Sol2 states, including per-script states identified by an owner script ID. `Lua_Script_Manager` loads scripts through `safe_script_file`, tracks script state/errors, and removes script-owned commands before destroying the engine. `Lua_Module_Manager` discovers and loads reusable `.lua` modules. Binding libraries are registered independently and applied to every engine.

The first core binding library exposes `devilz.api_version`, runtime/engine metadata, `devilz.version()`, and a script-owned `devilz.commands` API. Game bindings are intentionally not exposed in this foundation pass.

Lua states currently open the base, coroutine, math, string, table, and UTF-8 libraries. Lua-side `dofile` and `loadfile` are disabled; filesystem, OS, debug, and unrestricted native access remain unavailable.

Next layers can add controlled libraries for features, events, UI, players, entities, vehicles, weapons, configuration, and logging without turning `Lua_Manager` into a monolithic binding class.
