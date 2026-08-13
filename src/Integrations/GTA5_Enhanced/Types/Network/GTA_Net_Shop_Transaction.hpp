#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace Devilz::Integrations::GTA5_Enhanced
{
// Non-owning ABI view of the verified CNetShopTransaction prefix. The GTA
// object continues beyond this prefix, so this type must never be constructed
// as a replacement game object or destroyed through its virtual table.
struct alignas(8) GTA_Net_Shop_Transaction_View
{
    static constexpr std::size_t KnownPrefixSize = 0x30;

    void* virtualTable = nullptr;                  // 0x00
    std::int32_t transactionId = 0;               // 0x08
    std::int32_t type = 0;                        // 0x0C: basket or service
    std::array<std::byte, 0x08> unknown10{};       // 0x10
    std::int32_t category = 0;                    // 0x18
    std::int32_t status = 0;                      // 0x1C
    std::array<std::byte, 0x04> unknown20{};       // 0x20
    std::int32_t action = 0;                      // 0x24
    std::int32_t flags = 0;                       // 0x28
    std::uint8_t running = 0;                     // 0x2C
    std::array<std::byte, 0x03> unknown2D{};       // 0x2D

    [[nodiscard]] bool HasVirtualTable() const noexcept
    {
        return virtualTable != nullptr;
    }

    [[nodiscard]] bool Running() const noexcept
    {
        return running != 0;
    }

    [[nodiscard]] bool HasAnyFlags(std::uint32_t mask) const noexcept
    {
        return (static_cast<std::uint32_t>(flags) & mask) != 0;
    }

    [[nodiscard]] bool HasAllFlags(std::uint32_t mask) const noexcept
    {
        return (static_cast<std::uint32_t>(flags) & mask) == mask;
    }
};

static_assert(sizeof(void*) == 0x08);
static_assert(std::is_standard_layout_v<GTA_Net_Shop_Transaction_View>);
static_assert(std::is_trivially_copyable_v<GTA_Net_Shop_Transaction_View>);
static_assert(alignof(GTA_Net_Shop_Transaction_View) == 0x08);
static_assert(offsetof(GTA_Net_Shop_Transaction_View, virtualTable) == 0x00);
static_assert(offsetof(GTA_Net_Shop_Transaction_View, transactionId) == 0x08);
static_assert(offsetof(GTA_Net_Shop_Transaction_View, type) == 0x0C);
static_assert(offsetof(GTA_Net_Shop_Transaction_View, category) == 0x18);
static_assert(offsetof(GTA_Net_Shop_Transaction_View, status) == 0x1C);
static_assert(offsetof(GTA_Net_Shop_Transaction_View, action) == 0x24);
static_assert(offsetof(GTA_Net_Shop_Transaction_View, flags) == 0x28);
static_assert(offsetof(GTA_Net_Shop_Transaction_View, running) == 0x2C);
static_assert(sizeof(GTA_Net_Shop_Transaction_View) ==
              GTA_Net_Shop_Transaction_View::KnownPrefixSize);
}
