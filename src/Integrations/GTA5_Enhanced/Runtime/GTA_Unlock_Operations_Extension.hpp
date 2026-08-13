#pragma once

#include "GTA_Unlock_Operations_State.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace UnlockOperationsDetail
{

// -----------------------------------------------------------------------------
// Native hashes
// -----------------------------------------------------------------------------

inline constexpr std::uint64_t StatGetInt =
    0x767FBC2AC802EF3DULL;

inline constexpr std::uint64_t StatGetBool =
    0x11B5E6D2AE73F48EULL;

inline constexpr std::uint64_t StatSetInt =
    0x1164A75E490C27B6ULL;

inline constexpr std::uint64_t StatSetBool =
    0xF1D0B0CE940F620DULL;

inline constexpr std::uint64_t GetPackedStatBoolCode =
    0xA6D3C21763E25496ULL;

inline constexpr std::uint64_t SetPackedStatBoolCode =
    0xA595AA1819B05EA0ULL;


// -----------------------------------------------------------------------------
// Runtime policy
//
// Packed and ordinary stat operations use validated native handlers.
//
// Tunable/script-global operations deliberately remain disabled. They should
// not be re-enabled until their mapping is independently verified for the
// current GTA V Enhanced build.
// -----------------------------------------------------------------------------

inline constexpr bool EnableStatusReads = true;
inline constexpr bool EnablePackedBoolOperations = true;
inline constexpr bool EnableStatOperations = true;
inline constexpr bool EnableTunableOperations = false;

inline constexpr std::size_t CommandsPerTick = 8;


// -----------------------------------------------------------------------------
// Hash helpers
// -----------------------------------------------------------------------------

[[nodiscard]] inline char Lower(char value) noexcept
{
    return value >= 'A' && value <= 'Z'
        ? static_cast<char>(value + ('a' - 'A'))
        : value;
}

[[nodiscard]] inline std::uint32_t Joaat(
    std::string_view text) noexcept
{
    std::uint32_t hash = 0;

    for (const char character : text) {
        hash += static_cast<std::uint8_t>(Lower(character));
        hash += hash << 10U;
        hash ^= hash >> 6U;
    }

    hash += hash << 3U;
    hash ^= hash >> 11U;

    return hash + (hash << 15U);
}


// -----------------------------------------------------------------------------
// Character helpers
// -----------------------------------------------------------------------------

template <typename NativeManager>
[[nodiscard]] bool ResolveCharacterSlot(
    NativeManager& natives,
    std::int32_t& slot) noexcept
{
    std::int32_t character = 0;

    const auto result = natives.template InvokeHash<bool>(
        StatGetInt,
        Joaat("MPPLY_LAST_MP_CHAR"),
        &character,
        -1);

    if (!result.has_value() || !result.value())
        return false;

    slot = character == 1 ? 1 : 0;
    return true;
}


// Packed-stat natives already support the current-character sentinel.
//
// Prefer an explicitly resolved slot when possible, but keep -1 as the
// fallback so packed operations do not depend on MPPLY_LAST_MP_CHAR being
// readable.
template <typename NativeManager>
[[nodiscard]] std::int32_t ResolvePackedCharacterSlot(
    NativeManager& natives) noexcept
{
    std::int32_t slot = -1;

    std::int32_t resolved = 0;
    if (ResolveCharacterSlot(natives, resolved))
        slot = resolved;

    return slot;
}


// -----------------------------------------------------------------------------
// Stat-name resolution
// -----------------------------------------------------------------------------

template <typename NativeManager>
[[nodiscard]] bool ResolveStatHash(
    NativeManager& natives,
    std::string name,
    std::uint32_t& hash) noexcept
{
    if (name.empty() || name.size() > 127)
        return false;

    // Normalize MPX_* to MP0_* / MP1_* using the active character.
    if (name.size() > 3 &&
        Lower(name[0]) == 'm' &&
        Lower(name[1]) == 'p' &&
        Lower(name[2]) == 'x' &&
        name[3] == '_') {

        std::int32_t characterSlot = 0;

        if (!ResolveCharacterSlot(natives, characterSlot))
            return false;

        name[2] = characterSlot == 1 ? '1' : '0';
    }

    hash = Joaat(name);
    return hash != 0;
}


// -----------------------------------------------------------------------------
// Completion helpers
// -----------------------------------------------------------------------------

inline void CompleteFailure(
    GTA_Unlock_Operations_State& state,
    const GTA_Unlock_Operations_State::Command& command,
    const char* detail) noexcept
{
    state.Complete(
        command,
        false,
        false,
        false,
        detail ? detail : "Unlock operation failed");
}


// Failure where we still know the resulting value did NOT match.
//
// This is useful for readback verification. The operation failed, but the
// state is known rather than unknown.
inline void CompleteVerifiedFailure(
    GTA_Unlock_Operations_State& state,
    const GTA_Unlock_Operations_State::Command& command,
    const char* detail) noexcept
{
    state.Complete(
        command,
        false,
        true,
        false,
        detail ? detail : "Unlock write did not persist");
}


// -----------------------------------------------------------------------------
// Packed bool operations
// -----------------------------------------------------------------------------

template <typename NativeManager>
void ExecutePackedBool(
    NativeManager& natives,
    GTA_Unlock_Operations_State& state,
    const GTA_Unlock_Operations_State::Command& command,
    bool write) noexcept
{
    const auto& operation = command.operation;

    const std::int32_t characterSlot =
        ResolvePackedCharacterSlot(natives);

    // Always read first.
    //
    // This lets us:
    //   1. answer status refreshes;
    //   2. detect already-unlocked items;
    //   3. avoid unnecessary writes.
    const auto current = natives.template InvokeHash<bool>(
        GetPackedStatBoolCode,
        operation.index,
        characterSlot);

    if (!current.has_value()) {
        CompleteFailure(
            state,
            command,
            "Packed-stat read handler unavailable");
        return;
    }

    const bool currentlyMatched =
        current.value() == operation.boolValue;

    // Status query.
    if (!write) {
        state.Complete(
            command,
            true,
            true,
            currentlyMatched,
            currentlyMatched
                ? "Packed stat matched"
                : "Packed stat did not match");

        return;
    }

    // Already unlocked / already at requested state.
    if (currentlyMatched) {
        state.Complete(
            command,
            true,
            true,
            true,
            "Packed stat already matched");

        return;
    }

    // Dispatch write.
    const bool dispatched =
        natives.template InvokeHash<void>(
            SetPackedStatBoolCode,
            operation.index,
            operation.boolValue,
            characterSlot);

    if (!dispatched) {
        CompleteFailure(
            state,
            command,
            "Packed-stat write handler unavailable");
        return;
    }

    // ---------------------------------------------------------------------
    // CRITICAL:
    //
    // InvokeHash<void>() only tells us that the native handler executed.
    // It does NOT prove GTA accepted the packed-stat change.
    //
    // Read the exact value back before reporting success.
    // ---------------------------------------------------------------------

    const auto verified =
        natives.template InvokeHash<bool>(
            GetPackedStatBoolCode,
            operation.index,
            characterSlot);

    if (!verified.has_value()) {
        CompleteFailure(
            state,
            command,
            "Packed-stat write dispatched but readback failed");
        return;
    }

    const bool matched =
        verified.value() == operation.boolValue;

    if (!matched) {
        CompleteVerifiedFailure(
            state,
            command,
            "Packed-stat write did not persist");
        return;
    }

    state.Complete(
        command,
        true,
        true,
        true,
        "Packed-stat write verified");
}


// -----------------------------------------------------------------------------
// Stat bool operations
// -----------------------------------------------------------------------------

template <typename NativeManager>
void ExecuteStatBool(
    NativeManager& natives,
    GTA_Unlock_Operations_State& state,
    const GTA_Unlock_Operations_State::Command& command,
    std::uint32_t statHash,
    bool write) noexcept
{
    const auto& operation = command.operation;

    std::int32_t current = 0;

    const auto read = natives.template InvokeHash<bool>(
        StatGetBool,
        statHash,
        &current,
        -1);

    if (!read.has_value() || !read.value()) {
        CompleteFailure(
            state,
            command,
            "STAT_GET_BOOL failed for this stat");
        return;
    }

    const bool currentValue = current != 0;
    const bool currentlyMatched =
        currentValue == operation.boolValue;

    // Status query.
    if (!write) {
        state.Complete(
            command,
            true,
            true,
            currentlyMatched,
            currentlyMatched
                ? "Stat bool matched"
                : "Stat bool did not match");

        return;
    }

    // Already at requested value.
    if (currentlyMatched) {
        state.Complete(
            command,
            true,
            true,
            true,
            "Stat bool already matched");

        return;
    }

    // Dispatch write.
    const bool dispatched =
        natives.template InvokeHash<void>(
            StatSetBool,
            statHash,
            operation.boolValue,
            true);

    if (!dispatched) {
        CompleteFailure(
            state,
            command,
            "STAT_SET_BOOL handler unavailable");
        return;
    }

    // Verify.
    std::int32_t verified = 0;

    const auto verify = natives.template InvokeHash<bool>(
        StatGetBool,
        statHash,
        &verified,
        -1);

    if (!verify.has_value() || !verify.value()) {
        CompleteFailure(
            state,
            command,
            "STAT_SET_BOOL dispatched but readback failed");
        return;
    }

    const bool matched =
        (verified != 0) == operation.boolValue;

    if (!matched) {
        CompleteVerifiedFailure(
            state,
            command,
            "Stat bool write did not persist");
        return;
    }

    state.Complete(
        command,
        true,
        true,
        true,
        "Stat bool write verified");
}


// -----------------------------------------------------------------------------
// Stat int operations
// -----------------------------------------------------------------------------

template <typename NativeManager>
void ExecuteStatInt(
    NativeManager& natives,
    GTA_Unlock_Operations_State& state,
    const GTA_Unlock_Operations_State::Command& command,
    std::uint32_t statHash,
    bool write) noexcept
{
    const auto& operation = command.operation;

    std::int32_t current = 0;

    const auto read = natives.template InvokeHash<bool>(
        StatGetInt,
        statHash,
        &current,
        -1);

    if (!read.has_value() || !read.value()) {
        CompleteFailure(
            state,
            command,
            "STAT_GET_INT failed for this stat");
        return;
    }

    const bool currentlyMatched =
        current == operation.intValue;

    // Status query.
    if (!write) {
        state.Complete(
            command,
            true,
            true,
            currentlyMatched,
            currentlyMatched
                ? "Stat int matched"
                : "Stat int did not match");

        return;
    }

    // Already at requested value.
    if (currentlyMatched) {
        state.Complete(
            command,
            true,
            true,
            true,
            "Stat int already matched");

        return;
    }

    // Dispatch write.
    const bool dispatched =
        natives.template InvokeHash<void>(
            StatSetInt,
            statHash,
            operation.intValue,
            true);

    if (!dispatched) {
        CompleteFailure(
            state,
            command,
            "STAT_SET_INT handler unavailable");
        return;
    }

    // Verify.
    std::int32_t verified = 0;

    const auto verify = natives.template InvokeHash<bool>(
        StatGetInt,
        statHash,
        &verified,
        -1);

    if (!verify.has_value() || !verify.value()) {
        CompleteFailure(
            state,
            command,
            "STAT_SET_INT dispatched but readback failed");
        return;
    }

    const bool matched =
        verified == operation.intValue;

    if (!matched) {
        CompleteVerifiedFailure(
            state,
            command,
            "Stat int write did not persist");
        return;
    }

    state.Complete(
        command,
        true,
        true,
        true,
        "Stat int write verified");
}


// -----------------------------------------------------------------------------
// Main command dispatcher
// -----------------------------------------------------------------------------

template <typename NativeManager>
void ExecuteCommand(
    NativeManager& natives,
    GTA_Unlock_Operations_State& state,
    const GTA_Unlock_Operations_State::Command& command) noexcept
{
    const auto& operation = command.operation;

    const bool write =
        command.kind ==
        GTA_Unlock_Operations_State::Command_Kind::Write;


    // ---------------------------------------------------------------------
    // Global read policy
    // ---------------------------------------------------------------------

    if (!write && !EnableStatusReads) {
        CompleteFailure(
            state,
            command,
            "Unlock status reads are disabled by runtime safety policy");
        return;
    }


    // ---------------------------------------------------------------------
    // Packed bool
    // ---------------------------------------------------------------------

    if (operation.type ==
        GTA_Unlock_Operation_Type::PackedBool) {

        if (!EnablePackedBoolOperations) {
            CompleteFailure(
                state,
                command,
                "Packed-bool unlock operations are disabled by runtime safety policy");
            return;
        }

        ExecutePackedBool(
            natives,
            state,
            command,
            write);

        return;
    }


    // ---------------------------------------------------------------------
    // Stat bool / int
    // ---------------------------------------------------------------------

    if (operation.type ==
            GTA_Unlock_Operation_Type::StatBool ||
        operation.type ==
            GTA_Unlock_Operation_Type::StatInt) {

        if (!EnableStatOperations) {
            CompleteFailure(
                state,
                command,
                "Stat unlock operations are disabled by runtime safety policy");
            return;
        }

        std::uint32_t statHash = 0;

        if (!ResolveStatHash(
                natives,
                operation.statName,
                statHash)) {

            CompleteFailure(
                state,
                command,
                "Stat name or active MP character could not be resolved");
            return;
        }

        if (operation.type ==
            GTA_Unlock_Operation_Type::StatBool) {

            ExecuteStatBool(
                natives,
                state,
                command,
                statHash,
                write);

            return;
        }

        ExecuteStatInt(
            natives,
            state,
            command,
            statHash,
            write);

        return;
    }


    // ---------------------------------------------------------------------
    // Tunable/script-global
    //
    // Do NOT access script globals here.
    //
    // A status refresh should not poison an otherwise successful Clothing
    // refresh simply because one catalog entry uses an unavailable tunable.
    //
    // Writes remain fail-closed.
    // ---------------------------------------------------------------------

    if (operation.type ==
        GTA_Unlock_Operation_Type::TunableInt) {

        if (!write) {
            state.Complete(
                command,
                true,
                false,
                false,
                EnableTunableOperations
                    ? "Tunable status path is not implemented"
                    : "Tunable status unavailable on this build");

            return;
        }

        CompleteFailure(
            state,
            command,
            EnableTunableOperations
                ? "Tunable write path is not implemented"
                : "Tunable unlock operations are disabled by runtime safety policy");

        return;
    }


    CompleteFailure(
        state,
        command,
        "Unsupported unlock operation type");
}

} // namespace UnlockOperationsDetail


// -----------------------------------------------------------------------------
// Game-thread queue service
// -----------------------------------------------------------------------------

template <typename NativeManager>
void TickUnlockOperationsExtension(
    NativeManager& natives) noexcept
{
    auto& state =
        GTA_Unlock_Operations_State::Instance();

    for (std::size_t processed = 0;
         processed < UnlockOperationsDetail::CommandsPerTick;
         ++processed) {

        GTA_Unlock_Operations_State::Command command{};

        if (!state.Consume(command))
            break;

        try {
            UnlockOperationsDetail::ExecuteCommand(
                natives,
                state,
                command);
        }
        catch (...) {
            UnlockOperationsDetail::CompleteFailure(
                state,
                command,
                "Unlock operation threw on the GTA game thread");
        }
    }
}

} // namespace Devilz::Integrations::GTA5_Enhanced
