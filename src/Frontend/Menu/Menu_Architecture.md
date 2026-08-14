# Devil's Den Menu Ownership

The frontend is organized by feature domain. New menu code belongs to the domain that owns it rather than a growing shared menu file.

```text
Frontend/Menu/
  Themes/                  reusable medieval theme, palette, header and texture drawing
  Self/                    player/self presentation
  Weapons/                 weapon/ammo/loadout presentation
  Vehicle/
    Spawner/                vehicle catalog and spawning UI
    Forge/
      Mods/
      Wheels/
      Paint/
      Lighting/
      Plates/
      Extras/
      Condition/
    SavedVehicles/
    Handling/
  Teleport/
    Waypoint/
    Locations/
  World/
  Network/
    Sessions/
    Players/
    Protections/
  Debug/                   raw developer tools such as the regular/packed stat editor
  Lua/                     script manager/console/settings presentation
  Settings/
    Configs/
    Theme/
    Input/
```

## Layer rule

- `Frontend/Menu/...` owns ImGui presentation only.
- `Integrations/GTA5_Enhanced/...` owns GTA Enhanced runtime/native behavior.
- `Scripting/Lua/...` will own the Lua VM, script lifecycle, events and bindings.
- Lua bindings should call the same backend feature APIs as the native menu instead of issuing unrelated duplicate native logic.

The existing `Devils_Den_Menu_part*.inc` files are legacy migration seams. Existing behavior stays stable while page bodies are moved domain-by-domain. Do not add new features to a legacy part file when a domain folder exists for that feature.

Examples:

- Session browser -> `Frontend/Menu/Network/Sessions/`
- Network player list -> `Frontend/Menu/Network/Players/`
- Side skirts and body mods -> `Frontend/Menu/Vehicle/Forge/Mods/`
- Benny's wheels -> `Frontend/Menu/Vehicle/Forge/Wheels/`
- Xenon/neon -> `Frontend/Menu/Vehicle/Forge/Lighting/`
- Vehicle JSON builds -> `Frontend/Menu/Vehicle/SavedVehicles/`
- Lua script list/console -> `Frontend/Menu/Lua/`
