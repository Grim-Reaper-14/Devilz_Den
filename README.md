# Devilz_Den

Devilz_Den is a Windows-first C++20 application runtime/framework built backend-first. The UI is a client of the runtime, not the runtime itself.

## Goals

- Direct3D 12 graphics backend
- Independent managed execution services for application components
- Structured error and result handling
- Full asynchronous logging with native Win32/HRESULT diagnostics
- Crash reporting, stack traces, and Windows minidumps
- Named executors, worker pools, IO execution, cancellation, and task diagnostics
- Managed backend services with lifecycle and health states
- Filesystem, resource, input, networking, and diagnostics services
- Deterministic startup and shutdown
- ImGui frontend after the backend runtime is stable

## Backend layout

```text
src/
  Backend/
    Error/
    Logging/
    Crash/
    Threading/
    Services/
    Diagnostics/
    Platform/Win32/
    Filesystem/
    Graphics/D3D12/
    Resources/
    Input/
    Network/
    Timing/
```

## Runtime rules

1. No unexplained `false` returns for fallible backend operations. Use structured `Result<T>` values.
2. Never discard a Win32 error, HRESULT, DXGI failure, or service failure.
3. Services do not create unmanaged threads. Execution resources come from the threading runtime.
4. Rendering and D3D12 ownership stay on their designated executor/thread.
5. Worker exceptions are captured and reported instead of disappearing.
6. Logging must not block the render path during normal operation.
7. Fatal errors synchronously flush diagnostics before termination when possible.
8. Every service exposes lifecycle state, health information, and its last failure.
9. Shutdown is deterministic and dependency-aware.

## Planned milestones

### Milestone 1 - Runtime foundation

- ErrorCode / Error / ErrorContext
- Result<T>
- LoggerService and structured LogRecord
- Console, debugger, file, memory, and crash sinks
- CrashHandler and minidump support
- ThreadManager
- Executor abstraction
- Shared worker, IO, dedicated, main, and render executors
- Task IDs, cancellation, timing, and exception capture
- Backend service interface and lifecycle state

### Milestone 2 - Platform and IO

- Win32 platform service
- Filesystem service
- directory watching
- asynchronous IO
- runtime diagnostics registry
- watchdog and slow-task reporting

### Milestone 3 - D3D12

- adapter/device selection
- command queues and frame contexts
- descriptor allocation
- swap chain and resize handling
- fences and deferred destruction
- upload service
- D3D12/DXGI debug message integration
- device removal diagnostics
- CPU/GPU timing

### Milestone 4 - Higher-level services

- resource/asset service
- input service
- networking service
- configuration service
- scripting service

### Milestone 5 - Core and frontend

Once the backend is stable, Core will orchestrate the runtime and an ImGui/D3D12 frontend will expose logs, services, threads, tasks, errors, resources, and performance diagnostics.

## Toolchain

- Windows 10/11
- Visual Studio 2022 / MSVC v143
- C++20
- CMake
- x64
- Direct3D 12 / DXGI

## Status

Fresh implementation. The backend runtime is being built before Core and Frontend.
