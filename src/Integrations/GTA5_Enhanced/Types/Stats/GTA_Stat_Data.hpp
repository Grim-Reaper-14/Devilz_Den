#pragma once

#include <cstddef>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Stat_Data
{
public:
    enum class Type : std::int32_t
    {
        Int = 1,
        Float,
        String,
        Bool,
        UInt8,
        UInt16,
        UInt32,
        UInt64,
        Date = 20,
        Pos,
        Int64 = 26,
    };

    // Non-owning ABI interface over a game-owned stat object. Do not construct
    // or destroy these objects from Devilz Den; prefer stat natives for writes.
    virtual ~GTA_Stat_Data() = default;                           // vtbl +0x00
    virtual void Reset() = 0;                                    // vtbl +0x08
    virtual void SetInt(std::int32_t value) = 0;                  // vtbl +0x10
    virtual void SetInt64(std::int64_t value) = 0;                // vtbl +0x18
    virtual void SetFloat(float value) = 0;                       // vtbl +0x20
    virtual void SetBool(bool value) = 0;                         // vtbl +0x28
    virtual void SetUInt8(std::uint8_t value) = 0;                // vtbl +0x30
    virtual void SetUInt16(std::uint16_t value) = 0;              // vtbl +0x38
    virtual void SetUInt32(std::uint32_t value) = 0;              // vtbl +0x40
    virtual void SetUInt64(std::uint64_t value) = 0;              // vtbl +0x48
    virtual void SetString(const char* value) = 0;                // vtbl +0x50
    virtual bool SetUserId(const char* value) = 0;                // vtbl +0x58
    virtual std::int32_t GetInt() = 0;                            // vtbl +0x60
    virtual std::int64_t GetInt64() = 0;                          // vtbl +0x68
    virtual float GetFloat() = 0;                                 // vtbl +0x70
    virtual bool GetBool() = 0;                                   // vtbl +0x78
    virtual std::uint8_t GetUInt8() = 0;                          // vtbl +0x80
    virtual std::uint16_t GetUInt16() = 0;                        // vtbl +0x88
    virtual std::uint32_t GetUInt32() = 0;                        // vtbl +0x90
    virtual std::uint64_t GetUInt64() = 0;                        // vtbl +0x98
    virtual const char* GetString() = 0;                           // vtbl +0xA0
    virtual bool GetUserId(char* buffer, std::int32_t size) = 0;  // vtbl +0xA8
    virtual bool GetProfileSetting(void* buffer) = 0;             // vtbl +0xB0
    virtual bool IsEqual(std::uint64_t* value) = 0;               // vtbl +0xB8
    virtual Type GetType() = 0;                                   // vtbl +0xC0
    virtual const char* GetTypeString() = 0;                       // vtbl +0xC8
    virtual std::int32_t GetSize() = 0;                           // vtbl +0xD0
    virtual bool IsObfuscated() = 0;                              // vtbl +0xD8
    virtual void UnkE0(bool value) = 0;                           // vtbl +0xE0
    virtual std::int32_t UnkE8() = 0;                             // vtbl +0xE8
    virtual bool IsValueNull() = 0;                               // vtbl +0xF0

    std::uint32_t flags = 0;

    [[nodiscard]] bool IsServerAuthoritative() const noexcept
    {
        return (flags & 0x00000080U) != 0;
    }

    [[nodiscard]] bool IsControlledByNetShop() const noexcept
    {
        return (flags & 0x08000000U) != 0;
    }
};

struct alignas(8) GTA_Stat_Map
{
    std::uint32_t hash = 0;
    std::uint32_t unknown04 = 0;
    GTA_Stat_Data* data = nullptr;
};

static_assert(sizeof(void*) == 0x08);
static_assert(sizeof(GTA_Stat_Data::Type) == 0x04);
static_assert(alignof(GTA_Stat_Data) == 0x08);
static_assert(sizeof(GTA_Stat_Data) == 0x10);
static_assert(alignof(GTA_Stat_Map) == 0x08);
static_assert(offsetof(GTA_Stat_Map, hash) == 0x00);
static_assert(offsetof(GTA_Stat_Map, unknown04) == 0x04);
static_assert(offsetof(GTA_Stat_Map, data) == 0x08);
static_assert(sizeof(GTA_Stat_Map) == 0x10);
}
