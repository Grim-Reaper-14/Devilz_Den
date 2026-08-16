# GTA V Enhanced decompiled scripts

Devilz_Den treats the decompiled GTA V Enhanced scripts as external developer reference data. They are not compiled into the DLL and are not copied into runtime output.

The source is pinned to:

- Repository: `https://github.com/acidlabsdev/gtav-enhanced-scripts.git`
- Revision: `30dd0df8bce87bdc21103555bae380be9fc0a916`
- Upstream game target: GTA Online 1.73 / Enhanced build 1158.13

## Sync the reference checkout

Configure Devilz_Den normally, then run:

```text
cmake --build build --target Devilz_Den_Sync_DecompileScripts
```

The checkout is placed in `decompileScripts/` by default. That directory is ignored by Devilz_Den so generated/decompiled upstream files cannot be committed accidentally.

To keep a separate checkout, configure with:

```text
-DDEVILZ_DECOMPILE_SCRIPTS_DIR=C:/path/to/decompileScripts
```

The sync script refuses to overwrite a non-empty non-Git directory or a Git checkout whose `origin` does not match the configured repository.

When upstream updates for a new GTA Enhanced build, validate the scripts against the target game build first, then update `DEVILZ_DECOMPILE_SCRIPTS_REVISION` in `CMakeLists.txt` deliberately.
