#pragma once

#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <utility>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
inline constexpr std::size_t GTA_Outfit_Component_Count = 12;
inline constexpr std::size_t GTA_Outfit_Prop_Count = 5;

struct GTA_Outfit_Item_State
{
    int slot = 0;
    int drawable = 0;
    int drawableMax = 0;
    int texture = 0;
    int textureMax = 0;
    int palette = 0;
    bool valid = false;
};

struct GTA_Outfit_Editor_Snapshot
{
    std::uint64_t generation = 0;
    int ped = 0;
    std::uint32_t modelHash = 0;
    bool ready = false;
    std::array<GTA_Outfit_Item_State, GTA_Outfit_Component_Count> components{};
    std::array<GTA_Outfit_Item_State, GTA_Outfit_Prop_Count> props{};
};

namespace OutfitEditorDetail
{
constexpr GTA_Native_Hash SetPedComponentVariationHash = 0xD1C578C204015E1FULL;
constexpr GTA_Native_Hash GetPedDrawableVariationHash = 0xC0120BBCC298EA2FULL;
constexpr GTA_Native_Hash GetNumberOfPedDrawableVariationsHash = 0x1A4EFE92822E3123ULL;
constexpr GTA_Native_Hash GetPedTextureVariationHash = 0xD6AED6BFCC58AF7FULL;
constexpr GTA_Native_Hash GetNumberOfPedTextureVariationsHash = 0x8401C77F508D70FDULL;
constexpr GTA_Native_Hash GetPedPaletteVariationHash = 0xDAF263B0E792EAECULL;
constexpr GTA_Native_Hash GetPedPropIndexHash = 0xB204F40D393426B6ULL;
constexpr GTA_Native_Hash GetNumberOfPedPropDrawableVariationsHash = 0x4D0F04723A52D0E9ULL;
constexpr GTA_Native_Hash GetPedPropTextureIndexHash = 0x0DC23FA727759F9FULL;
constexpr GTA_Native_Hash GetNumberOfPedPropTextureVariationsHash = 0x1D77F90D87ACD2BAULL;
constexpr GTA_Native_Hash SetPedPropIndexHash = 0x7F08C4791E6D6969ULL;
constexpr GTA_Native_Hash ClearPedPropHash = 0x09397806857F5DFBULL;

constexpr std::array<int, GTA_Outfit_Component_Count> ComponentSlots{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

constexpr std::array<int, GTA_Outfit_Prop_Count> PropSlots{
    0, 1, 2, 6, 7
};

enum class CommandType : std::uint8_t
{
    Component,
    Prop
};

struct Command
{
    CommandType type = CommandType::Component;
    int targetPed = 0;
    int slot = 0;
    int drawable = 0;
    int texture = 0;
    int palette = -1;
};

struct Storage
{
    std::mutex mutex;
    GTA_Outfit_Editor_Snapshot snapshot{};
    std::vector<Command> commands;
    std::atomic_bool refreshRequested{true};
    int lastPed = 0;
    std::uint32_t lastModelHash = 0;
};

inline Storage& State() noexcept
{
    static Storage state;
    return state;
}

inline int MaxFromCount(int count, int emptyMax = 0) noexcept
{
    return count > 0 ? count - 1 : emptyMax;
}

inline void Queue(Command command) noexcept
{
    auto& state = State();
    std::scoped_lock lock(state.mutex);
    command.targetPed = state.snapshot.ped;
    if (state.commands.size() >= 64)
        state.commands.erase(state.commands.begin());
    state.commands.push_back(command);
}

template <typename NativeManager>
void RefreshSnapshot(NativeManager& natives, int ped, std::uint32_t modelHash) noexcept
{
    GTA_Outfit_Editor_Snapshot refreshed{};
    refreshed.ped = ped;
    refreshed.modelHash = modelHash;
    refreshed.ready = ped != 0;

    if (ped != 0) {
        for (std::size_t index = 0; index < ComponentSlots.size(); ++index) {
            const int slot = ComponentSlots[index];
            auto& item = refreshed.components[index];
            item.slot = slot;

            const auto drawable = natives.template InvokeHash<int>(GetPedDrawableVariationHash, ped, slot);
            const auto drawableCount = natives.template InvokeHash<int>(GetNumberOfPedDrawableVariationsHash, ped, slot);
            item.drawable = drawable.value_or(0);
            item.drawableMax = MaxFromCount(drawableCount.value_or(0));

            const auto texture = natives.template InvokeHash<int>(GetPedTextureVariationHash, ped, slot);
            const auto textureCount = natives.template InvokeHash<int>(
                GetNumberOfPedTextureVariationsHash,
                ped,
                slot,
                item.drawable);
            item.texture = texture.value_or(0);
            item.textureMax = MaxFromCount(textureCount.value_or(0));

            const auto palette = natives.template InvokeHash<int>(GetPedPaletteVariationHash, ped, slot);
            item.palette = palette.value_or(0);
            item.valid = drawable.has_value() && drawableCount.has_value() && texture.has_value() &&
                textureCount.has_value() && palette.has_value();
        }

        for (std::size_t index = 0; index < PropSlots.size(); ++index) {
            const int slot = PropSlots[index];
            auto& item = refreshed.props[index];
            item.slot = slot;

            const auto drawable = natives.template InvokeHash<int>(GetPedPropIndexHash, ped, slot, 0);
            const auto drawableCount = natives.template InvokeHash<int>(
                GetNumberOfPedPropDrawableVariationsHash,
                ped,
                slot);
            item.drawable = drawable.value_or(-1);
            item.drawableMax = MaxFromCount(drawableCount.value_or(0), -1);

            const auto texture = natives.template InvokeHash<int>(GetPedPropTextureIndexHash, ped, slot);
            item.texture = item.drawable >= 0 ? texture.value_or(0) : 0;
            if (item.drawable >= 0) {
                const auto textureCount = natives.template InvokeHash<int>(
                    GetNumberOfPedPropTextureVariationsHash,
                    ped,
                    slot,
                    item.drawable);
                item.textureMax = MaxFromCount(textureCount.value_or(0));
                item.valid = drawable.has_value() && drawableCount.has_value() &&
                    texture.has_value() && textureCount.has_value();
            } else {
                item.textureMax = 0;
                item.valid = drawable.has_value() && drawableCount.has_value();
            }
        }
    }

    auto& state = State();
    std::scoped_lock lock(state.mutex);
    refreshed.generation = state.snapshot.generation + 1;
    state.snapshot = std::move(refreshed);
}

template <typename NativeManager>
bool ApplyComponent(NativeManager& natives, int ped, const Command& command) noexcept
{
    const auto drawableCount = natives.template InvokeHash<int>(
        GetNumberOfPedDrawableVariationsHash,
        ped,
        command.slot);
    if (!drawableCount || *drawableCount <= 0)
        return false;

    const int drawable = std::clamp(command.drawable, 0, *drawableCount - 1);
    const auto textureCount = natives.template InvokeHash<int>(
        GetNumberOfPedTextureVariationsHash,
        ped,
        command.slot,
        drawable);
    const int textureMax = textureCount && *textureCount > 0 ? *textureCount - 1 : 0;
    const int texture = std::clamp(command.texture, 0, textureMax);

    int palette = command.palette;
    if (palette < 0) {
        const auto currentPalette = natives.template InvokeHash<int>(GetPedPaletteVariationHash, ped, command.slot);
        palette = currentPalette.value_or(0);
    }

    return natives.template InvokeHash<void>(
        SetPedComponentVariationHash,
        ped,
        command.slot,
        drawable,
        texture,
        palette);
}

template <typename NativeManager>
bool ApplyProp(NativeManager& natives, int ped, const Command& command) noexcept
{
    if (command.drawable < 0)
        return natives.template InvokeHash<void>(ClearPedPropHash, ped, command.slot, 1);

    const auto drawableCount = natives.template InvokeHash<int>(
        GetNumberOfPedPropDrawableVariationsHash,
        ped,
        command.slot);
    if (!drawableCount || *drawableCount <= 0)
        return false;

    const int drawable = std::clamp(command.drawable, 0, *drawableCount - 1);
    const auto textureCount = natives.template InvokeHash<int>(
        GetNumberOfPedPropTextureVariationsHash,
        ped,
        command.slot,
        drawable);
    const int textureMax = textureCount && *textureCount > 0 ? *textureCount - 1 : 0;
    const int texture = std::clamp(command.texture, 0, textureMax);

    return natives.template InvokeHash<void>(
        SetPedPropIndexHash,
        ped,
        command.slot,
        drawable,
        texture,
        true,
        0);
}
}

[[nodiscard]] inline GTA_Outfit_Editor_Snapshot OutfitEditorSnapshot() noexcept
{
    auto& state = OutfitEditorDetail::State();
    std::scoped_lock lock(state.mutex);
    return state.snapshot;
}

inline void RequestOutfitEditorRefresh() noexcept
{
    OutfitEditorDetail::State().refreshRequested.store(true, std::memory_order_release);
}

inline void RequestOutfitComponentChange(int slot, int drawable, int texture, int palette) noexcept
{
    OutfitEditorDetail::Queue({
        OutfitEditorDetail::CommandType::Component,
        0,
        slot,
        drawable,
        texture,
        palette
    });
}

inline void RequestOutfitPropChange(int slot, int drawable, int texture) noexcept
{
    OutfitEditorDetail::Queue({
        OutfitEditorDetail::CommandType::Prop,
        0,
        slot,
        drawable,
        texture,
        0
    });
}

inline void ResetOutfitEditorExtension() noexcept
{
    auto& state = OutfitEditorDetail::State();
    {
        std::scoped_lock lock(state.mutex);
        state.snapshot = {};
        state.commands.clear();
    }
    state.lastPed = 0;
    state.lastModelHash = 0;
    state.refreshRequested.store(true, std::memory_order_release);
}

template <typename NativeManager>
void TickOutfitEditorExtension(NativeManager& natives) noexcept
{
    using namespace OutfitEditorDetail;

    const auto pedResult = natives.template Invoke<int>(GTA_Native_Id::PlayerPedId);
    const int ped = pedResult.value_or(0);
    if (ped == 0) {
        auto& state = State();
        if (state.lastPed != 0)
            ResetOutfitEditorExtension();
        return;
    }

    const auto modelResult = natives.template Invoke<std::uint32_t>(GTA_Native_Id::GetEntityModel, ped);
    const std::uint32_t modelHash = modelResult.value_or(0);

    std::vector<Command> commands;
    {
        auto& state = State();
        std::scoped_lock lock(state.mutex);
        commands.swap(state.commands);
    }

    bool applied = false;
    for (const auto& command : commands) {
        if (command.targetPed != 0 && command.targetPed != ped)
            continue;
        if (command.type == CommandType::Component)
            applied = ApplyComponent(natives, ped, command) || applied;
        else
            applied = ApplyProp(natives, ped, command) || applied;
    }

    auto& state = State();
    const bool identityChanged = ped != state.lastPed || modelHash != state.lastModelHash;
    const bool refresh = state.refreshRequested.exchange(false, std::memory_order_acq_rel) ||
        identityChanged || applied;

    state.lastPed = ped;
    state.lastModelHash = modelHash;

    if (refresh)
        RefreshSnapshot(natives, ped, modelHash);
}
}
