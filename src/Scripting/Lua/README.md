# Future Lua Runtime

This directory is reserved for the embedded Devil's Den Lua runtime.

Planned ownership:

```text
Scripting/Lua/
  Lua_Manager.*
  Lua_Context.*
  Lua_Script.*
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

The Lua runtime should expose controlled feature APIs and events while keeping GTA Enhanced native execution inside the validated runtime/game-thread layer.

Expected lifecycle: discover scripts, load, execute, coroutine/tick scheduling, reload, unload, error reporting, and per-script settings.
