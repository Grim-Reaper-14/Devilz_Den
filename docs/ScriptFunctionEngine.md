# Verified Script Function Engine

Devilz_Den routes Rockstar script-function execution through one shared, verified ScriptVM engine.
The decompiled Enhanced scripts are reference/provenance data; the runtime always executes the live
`rage::scrProgram` bytecode loaded by GTA.

## Safety model

Every descriptor must identify the target script, GTA build fingerprint, decompile source, synchronous
execution contract, function ABI, and a sufficiently strong byte signature. Optional script code-size
checks can add another gate. Resolved program counters are cached only in memory and are signature-
revalidated before every cache hit. A script reload, code-block change, descriptor change, signature
change, or build change invalidates or rejects the cached entry.

Only calls running from the existing `RunScriptThreads` bridge may enter Rockstar ScriptVM. The target
thread TLS context is swapped temporarily, the unused live thread-stack tail is preserved, arguments and
a synthetic return PC are written, ScriptVM executes with a private context copy, the return slot is
captured, and the original stack/TLS state is restored. Descriptors are synchronous-only until a
separate yielding/coroutine contract is implemented.

## Locator modes

- `FixedProgramCounter`: verify a signature at a known entry PC.
- `UniqueBytePattern`: scan once for an entry signature.
- `UniqueBytePatternU24Target`: find a unique call-site signature and decode the 24-bit target PC from
  the verified bytecode operand. This matches Rockstar script call patterns already used by Devilz_Den.

## ABI

The engine supports typed scalar script slots:

- `Int32`
- `UInt32`
- `Int64`
- `UInt64`
- `Float`
- `Bool`
- `Hash32`
- `Void` returns

Argument count and type are validated before the live script stack is touched. Return values are captured
from the original top-of-stack slot before the temporary stack state is restored.

## Queue and performance

Menu/Lua calls use a bounded 16-request FIFO. One request is drained per `RunScriptThreads` dispatch so a
burst cannot monopolize a game frame. Idle overhead is an atomic pending check. Pattern scans happen only
on a request and only after a cache miss. The cache is revalidated rather than trusted blindly.

The engine maintains a bounded completion history and metrics for queueing, rejection, execution,
failures, cache hits/misses, pattern scans, and signature revalidations.

## Descriptor registry

Trusted C++ can register descriptors with either `Internal` or `UserFacing` exposure. Internal descriptors
remain callable only from trusted C++ APIs. Menu/Lua surfaces must use `RequestUserFacingScriptFunction`,
which rejects internal descriptors even when their ids are known.

Use stable ids such as:

```
recovery.xmas.unlock_facepaint_12
network.shop_controller.send_to_clouds
```

Structured source metadata should include the decompile repository, pinned revision, script path, and
function name when known.

## Lua surface

The core Lua table exposes a restricted `devilz.script_vm` namespace:

- `available()`
- `list()`
- `invoke(id, args_table)`
- `status(request_id)`
- `metrics()`

Lua cannot provide a raw PC, script hash, build override, or byte pattern. Only C++-registered
`UserFacing` descriptors can be queued.

Example after a verified user-facing descriptor is registered:

```lua
local request_id, message = devilz.script_vm.invoke(
    "recovery.example.verified_function",
    { 42, true }
)

if request_id ~= 0 then
    local result = devilz.script_vm.status(request_id)
    -- Poll from later ticks until result.done is true.
end
```

## Adding an unlock routine

1. Trace the effect in the pinned Enhanced decompile.
2. Prefer reproducing a small verified stat/global write when that is the entire effect.
3. Use ScriptVM only when the Rockstar helper performs meaningful logic that should remain authoritative.
4. Determine the exact ABI and confirm the function is synchronous.
5. Create a build-specific descriptor with a unique signature/call-site and decompile provenance.
6. Register it as `Internal` first and test the side effects.
7. Promote it to `UserFacing` only after the result is understood and repeatable.

This keeps decompile research, runtime execution, menu UI, and Lua automation on one guarded backend.
