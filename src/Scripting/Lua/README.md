# Lua Runtime

The first Sol2 integration owns a single embedded Lua 5.4 state. The Lua menu page initializes it lazily and can run a protected self-test. The state currently opens the base, coroutine, math, string, table, and UTF-8 libraries; filesystem, operating-system, debug, and game bindings are not exposed.

Planned ownership:

```text
Scripting/Lua/
  Lua_Runtime.*
  Lua_Manager.*          # future script discovery and ownership
  Lua_Context.*          # future per-script environment
  Lua_Script.*           # future loaded-script model
  Events/
  Bindings/
    Self/
    Weapons/
    Vehicle/
    Teleport/
    World/
    Network/
  API/
```

Future Lua bindings should expose controlled feature APIs and events while keeping GTA Enhanced native execution inside the validated runtime/game-thread layer.

Expected lifecycle: discover scripts, load, execute, coroutine/tick scheduling, reload, unload, error reporting, and per-script settings.
