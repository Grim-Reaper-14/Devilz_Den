# Lua Config Persistence

`Lua_Config_Manager` gives each loaded script a persistent config namespace without exposing arbitrary filesystem access to Lua.

Each script namespace is derived from a sanitized script filename plus a deterministic hash of its normalized path. Script content changes and hot reloads therefore keep the same config namespace, while unrelated scripts remain isolated.

Profiles are written under:

```text
%LOCALAPPDATA%/Devilz_Den/Lua/Configs/<script-key>/<profile>.json
```

When `LOCALAPPDATA` is unavailable, the runtime falls back to `./Devilz_Den/Lua/Configs`.

Profile names may contain only letters, numbers, `-`, and `_`, with a maximum length of 64 characters. Lua never supplies a raw path.

Only values registered through `devilz.settings` are persisted. Supported types remain boolean, integer, number, and string. Loading a profile updates only settings that are currently registered by the same script and still have the same type; stale or foreign keys are ignored.

```lua
assert(devilz.settings.register("enabled", false))
assert(devilz.settings.register("scale", 1.0))

local ok, saved, message = devilz.config.save("default")
print(ok, saved, message)

local loaded, applied, load_message = devilz.config.load("default")
print(loaded, applied, load_message)

for _, profile in ipairs(devilz.config.list()) do
    print(profile)
end

local removed, remove_message = devilz.config.remove("default")
print(removed, remove_message)
```

`devilz.config.script_key` is the isolated non-sensitive namespace identifier. Config owner attachment survives hot reload for the same script owner and is detached when the script is fully unloaded. Persistent profile files are intentionally not deleted on unload.
