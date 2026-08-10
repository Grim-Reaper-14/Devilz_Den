#include "Backend/Process/Process_Memory_Reader.hpp"
#include "Integrations/GTA5_Enhanced/Memory/GTA_Address_Resolver.hpp"

#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>

namespace
{
using Devilz::Backend::Process_Memory_Reader;
using Devilz::Integrations::GTA5_Enhanced::GTA_Address_Resolve_Chain;
using Devilz::Integrations::GTA5_Enhanced::GTA_Address_Resolve_Op_Type;
using Devilz::Integrations::GTA5_Enhanced::GTA_Address_Resolver;

bool ExpectAddress(
    const char* name,
    const Devilz::Backend::Result<std::uintptr_t>& result,
    std::uintptr_t expected)
{
    if (!result) {
        std::cerr << name << " failed: " << result.Failure().DetailedDescription() << '\n';
        return false;
    }

    if (result.Value() != expected) {
        std::cerr << name << " resolved 0x" << std::hex << result.Value()
                  << " but expected 0x" << expected << std::dec << '\n';
        return false;
    }

    return true;
}

bool WriteRipDisplacement(
    std::uintptr_t instruction,
    std::ptrdiff_t displacementOffset,
    std::uintptr_t target)
{
    const auto displacementAddress =
        static_cast<std::intptr_t>(instruction) + displacementOffset;
    const auto instructionEnd =
        displacementAddress + static_cast<std::intptr_t>(sizeof(std::int32_t));
    const auto delta =
        static_cast<std::intptr_t>(target) - instructionEnd;

    if (delta < std::numeric_limits<std::int32_t>::min() ||
        delta > std::numeric_limits<std::int32_t>::max()) {
        return false;
    }

    const auto displacement = static_cast<std::int32_t>(delta);
    std::memcpy(
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(displacementAddress)),
        &displacement,
        sizeof(displacement));
    return true;
}

bool TestEmptyAndAdd(const Process_Memory_Reader& reader)
{
    std::array<std::byte, 64> storage{};
    const auto match = reinterpret_cast<std::uintptr_t>(storage.data() + 16);

    if (!ExpectAddress(
            "empty chain",
            GTA_Address_Resolver::Resolve(reader, match, {}),
            match)) {
        return false;
    }

    const GTA_Address_Resolve_Chain addChain{
        {GTA_Address_Resolve_Op_Type::Add, 12},
        {GTA_Address_Resolve_Op_Type::Add, -4}
    };

    return ExpectAddress(
        "add chain",
        GTA_Address_Resolver::Resolve(reader, match, addChain),
        match + 8);
}

bool TestScriptThreadsRip(const Process_Memory_Reader& reader)
{
    std::array<std::byte, 128> storage{};
    const auto match = reinterpret_cast<std::uintptr_t>(storage.data() + 24);
    const auto target = reinterpret_cast<std::uintptr_t>(storage.data() + 104);

    if (!WriteRipDisplacement(match, 3, target)) {
        std::cerr << "ScriptThreads RIP displacement did not fit int32\n";
        return false;
    }

    const GTA_Address_Resolve_Chain chain{
        {GTA_Address_Resolve_Op_Type::RipRelative32, 3}
    };

    return ExpectAddress(
        "ScriptThreads RIP chain",
        GTA_Address_Resolver::Resolve(reader, match, chain),
        target);
}

bool TestScriptGlobalsAddRip(const Process_Memory_Reader& reader)
{
    std::array<std::byte, 160> storage{};
    const auto match = reinterpret_cast<std::uintptr_t>(storage.data() + 16);
    const auto instruction = match + 7;
    const auto target = reinterpret_cast<std::uintptr_t>(storage.data() + 136);

    if (!WriteRipDisplacement(instruction, 3, target)) {
        std::cerr << "ScriptGlobals RIP displacement did not fit int32\n";
        return false;
    }

    const GTA_Address_Resolve_Chain chain{
        {GTA_Address_Resolve_Op_Type::Add, 7},
        {GTA_Address_Resolve_Op_Type::RipRelative32, 3}
    };

    return ExpectAddress(
        "ScriptGlobals Add+RIP chain",
        GTA_Address_Resolver::Resolve(reader, match, chain),
        target);
}

bool TestRunScriptThreadsBacktrack(const Process_Memory_Reader& reader)
{
    std::array<std::byte, 64> storage{};
    const auto entry = reinterpret_cast<std::uintptr_t>(storage.data() + 16);
    const auto match = entry + 0xA;

    const GTA_Address_Resolve_Chain chain{
        {GTA_Address_Resolve_Op_Type::Add, -0xA}
    };

    return ExpectAddress(
        "RunScriptThreads backtrack",
        GTA_Address_Resolver::Resolve(reader, match, chain),
        entry);
}

bool TestInitNativeTablesBacktrack(const Process_Memory_Reader& reader)
{
    std::array<std::byte, 128> storage{};
    const auto entry = reinterpret_cast<std::uintptr_t>(storage.data() + 16);
    const auto match = entry + 0x2A;

    const GTA_Address_Resolve_Chain chain{
        {GTA_Address_Resolve_Op_Type::Add, -0x2A}
    };

    return ExpectAddress(
        "InitNativeTables backtrack",
        GTA_Address_Resolver::Resolve(reader, match, chain),
        entry);
}

bool TestNegativeRip(const Process_Memory_Reader& reader)
{
    std::array<std::byte, 160> storage{};
    const auto target = reinterpret_cast<std::uintptr_t>(storage.data() + 16);
    const auto instruction = reinterpret_cast<std::uintptr_t>(storage.data() + 104);

    if (!WriteRipDisplacement(instruction, 3, target)) {
        std::cerr << "Negative RIP displacement did not fit int32\n";
        return false;
    }

    const GTA_Address_Resolve_Chain chain{
        {GTA_Address_Resolve_Op_Type::RipRelative32, 3}
    };

    return ExpectAddress(
        "negative RIP chain",
        GTA_Address_Resolver::Resolve(reader, instruction, chain),
        target);
}
}

int main()
{
    Process_Memory_Reader reader(::GetCurrentProcessId());

    const bool passed =
        TestEmptyAndAdd(reader) &&
        TestScriptThreadsRip(reader) &&
        TestScriptGlobalsAddRip(reader) &&
        TestRunScriptThreadsBacktrack(reader) &&
        TestInitNativeTablesBacktrack(reader) &&
        TestNegativeRip(reader);

    if (!passed)
        return 1;

    std::cout << "GTA address resolver tests passed\n";
    return 0;
}
