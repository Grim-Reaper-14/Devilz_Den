#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string_view>
#include <type_traits>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Account_Platform : std::uint8_t
{
    Invalid = 0,
    Xbox = 1,
    Steam = 10,
    Epic = 15
};

[[nodiscard]] constexpr const char* GTA_Account_Platform_Name(
    GTA_Account_Platform platform) noexcept
{
    switch (platform) {
    case GTA_Account_Platform::Xbox: return "Xbox";
    case GTA_Account_Platform::Steam: return "Steam";
    case GTA_Account_Platform::Epic: return "Epic";
    case GTA_Account_Platform::Invalid:
    default: return "Invalid";
    }
}

struct alignas(8) GTA_Platform_Account_Id
{
    static constexpr std::size_t PayloadSize = 0x28;
    static constexpr std::size_t EpicAccountIdMaxLength = 32;

    std::array<std::byte, PayloadSize> payload{};
    GTA_Account_Platform platform = GTA_Account_Platform::Invalid;

    [[nodiscard]] constexpr bool Valid() const noexcept
    {
        return platform == GTA_Account_Platform::Xbox ||
               platform == GTA_Account_Platform::Steam ||
               platform == GTA_Account_Platform::Epic;
    }

    [[nodiscard]] constexpr bool Is(GTA_Account_Platform expected) const noexcept
    {
        return platform == expected;
    }

    [[nodiscard]] std::optional<std::uint64_t> XboxUserId() const noexcept
    {
        return NumericId(GTA_Account_Platform::Xbox);
    }

    [[nodiscard]] std::optional<std::uint64_t> SteamId() const noexcept
    {
        return NumericId(GTA_Account_Platform::Steam);
    }

    [[nodiscard]] std::string_view EpicAccountId() const noexcept
    {
        if (platform != GTA_Account_Platform::Epic)
            return {};

        const auto* text = reinterpret_cast<const char*>(payload.data());
        std::size_t length = 0;
        while (length < EpicAccountIdMaxLength && text[length] != '\0')
            ++length;
        return {text, length};
    }

    void SetXboxUserId(std::uint64_t value) noexcept
    {
        SetNumericId(GTA_Account_Platform::Xbox, value);
    }

    void SetSteamId(std::uint64_t value) noexcept
    {
        SetNumericId(GTA_Account_Platform::Steam, value);
    }

    [[nodiscard]] bool SetEpicAccountId(std::string_view value) noexcept
    {
        if (value.size() > EpicAccountIdMaxLength)
            return false;

        Reset();
        if (!value.empty())
            std::memcpy(payload.data(), value.data(), value.size());
        platform = GTA_Account_Platform::Epic;
        return true;
    }

    void Reset() noexcept
    {
        payload.fill(std::byte{0});
        platform = GTA_Account_Platform::Invalid;
    }

private:
    [[nodiscard]] std::optional<std::uint64_t> NumericId(
        GTA_Account_Platform expected) const noexcept
    {
        if (platform != expected)
            return std::nullopt;

        std::uint64_t value = 0;
        std::memcpy(&value, payload.data(), sizeof(value));
        return value;
    }

    void SetNumericId(GTA_Account_Platform target, std::uint64_t value) noexcept
    {
        Reset();
        std::memcpy(payload.data(), &value, sizeof(value));
        platform = target;
    }
};

static_assert(std::is_standard_layout_v<GTA_Platform_Account_Id>);
static_assert(std::is_trivially_copyable_v<GTA_Platform_Account_Id>);
static_assert(alignof(GTA_Platform_Account_Id) == 0x08);
static_assert(offsetof(GTA_Platform_Account_Id, payload) == 0x00);
static_assert(offsetof(GTA_Platform_Account_Id, platform) == 0x28);
static_assert(sizeof(GTA_Platform_Account_Id) == 0x30);
}
