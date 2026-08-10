#include "D3D12_Targets.hpp"

#include "Backend/Process/Process_Memory_Reader.hpp"
#include "Backend/Process/Process_Module_Manager.hpp"
#include "Backend/Process/Process_Pattern.hpp"
#include "Backend/Process/Process_Pattern_Scanner.hpp"
#include "Integrations/GTA5_Enhanced/Memory/GTA_Address_Resolver.hpp"

#include <cstring>

namespace Devilz::Frontend
{
using namespace Devilz::Backend;
using namespace Devilz::Integrations::GTA5_Enhanced;

namespace
{
template <typename T>
Result<T*> ReadPointer(const Process_Memory_Reader& reader, std::uintptr_t storage, const char* label)
{
    auto bytes = reader.Read(storage, sizeof(T*));
    if (!bytes)
        return Result<T*>::Failure(bytes.Failure());

    T* pointer = nullptr;
    std::memcpy(&pointer, bytes.Value().data(), sizeof(pointer));
    if (!pointer) {
        return Result<T*>::Failure(Error(
            ErrorCode::NotFound,
            ErrorCategory::Graphics,
            std::string(label) + " storage resolved to a null pointer"));
    }

    return Result<T*>::Success(pointer);
}
}

Result<D3D12_Targets> D3D12_Target_Resolver::Resolve(const GTA_Module_Status& status)
{
    if (!status.process || !status.build) {
        return Result<D3D12_Targets>::Failure(Error(
            ErrorCode::InvalidArgument,
            ErrorCategory::Graphics,
            "D3D12 target resolution requires a detected GTA process and build"));
    }

    if (status.build->fingerprint != SupportedFingerprint) {
        return Result<D3D12_Targets>::Failure(Error(
            ErrorCode::RuntimeFailure,
            ErrorCategory::Graphics,
            "No D3D12 graphics identity is registered for this GTA Enhanced build"));
    }

    Process_Module_Manager modules(status.process->pid);
    auto module = modules.Find("GTA5_Enhanced.exe");
    if (!module)
        return Result<D3D12_Targets>::Failure(module.Failure());

    const auto parsed = Process_Pattern::Parse(GraphicsPattern);
    if (!parsed) {
        return Result<D3D12_Targets>::Failure(Error(
            ErrorCode::RuntimeFailure,
            ErrorCategory::Graphics,
            "Registered GTA Enhanced D3D12 signature is invalid"));
    }

    Process_Pattern_Scanner scanner(status.process->pid);
    auto matches = scanner.Scan(module.Value(), *parsed, 2);
    if (!matches)
        return Result<D3D12_Targets>::Failure(matches.Failure());
    if (matches.Value().size() != 1) {
        return Result<D3D12_Targets>::Failure(Error(
            ErrorCode::RuntimeFailure,
            ErrorCategory::Graphics,
            "GTA Enhanced D3D12 signature did not resolve uniquely"));
    }

    Process_Memory_Reader reader(status.process->pid);
    const auto signature = matches.Value().front().address;

    const GTA_Address_Resolve_Chain commandQueueChain{
        {GTA_Address_Resolve_Op_Type::Add, 0x1A},
        {GTA_Address_Resolve_Op_Type::RipRelative32, 3}
    };
    const GTA_Address_Resolve_Chain swapChainChain{
        {GTA_Address_Resolve_Op_Type::Add, 0x21},
        {GTA_Address_Resolve_Op_Type::RipRelative32, 3}
    };

    auto commandQueueStorage = GTA_Address_Resolver::Resolve(reader, signature, commandQueueChain);
    if (!commandQueueStorage)
        return Result<D3D12_Targets>::Failure(commandQueueStorage.Failure());

    auto swapChainStorage = GTA_Address_Resolver::Resolve(reader, signature, swapChainChain);
    if (!swapChainStorage)
        return Result<D3D12_Targets>::Failure(swapChainStorage.Failure());

    auto commandQueue = ReadPointer<ID3D12CommandQueue>(reader, commandQueueStorage.Value(), "D3D12 command queue");
    if (!commandQueue)
        return Result<D3D12_Targets>::Failure(commandQueue.Failure());

    auto swapChain = ReadPointer<IDXGISwapChain1>(reader, swapChainStorage.Value(), "DXGI swap chain");
    if (!swapChain)
        return Result<D3D12_Targets>::Failure(swapChain.Failure());

    D3D12_Targets output{};
    output.signatureAddress = signature;
    output.commandQueueStorage = commandQueueStorage.Value();
    output.swapChainStorage = swapChainStorage.Value();

    if (FAILED(commandQueue.Value()->QueryInterface(IID_PPV_ARGS(output.commandQueue.ReleaseAndGetAddressOf())))) {
        return Result<D3D12_Targets>::Failure(Error(
            ErrorCode::D3D12Failure,
            ErrorCategory::Graphics,
            "Resolved command queue does not expose ID3D12CommandQueue"));
    }

    if (FAILED(swapChain.Value()->QueryInterface(IID_PPV_ARGS(output.swapChain.ReleaseAndGetAddressOf())))) {
        return Result<D3D12_Targets>::Failure(Error(
            ErrorCode::D3D12Failure,
            ErrorCategory::Graphics,
            "Resolved swap chain does not expose IDXGISwapChain3"));
    }

    return Result<D3D12_Targets>::Success(std::move(output));
}
}
