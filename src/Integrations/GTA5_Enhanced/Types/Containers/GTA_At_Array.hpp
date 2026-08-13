#pragma once

#include <cstddef>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
template <typename T>
struct alignas(8) GTA_At_Array_View
{
    T* data = nullptr;              // 0x00
    std::uint16_t size = 0;         // 0x08
    std::uint16_t capacity = 0;     // 0x0A

    [[nodiscard]] bool Empty() const noexcept
    {
        return size == 0;
    }

    [[nodiscard]] std::uint16_t Count() const noexcept
    {
        return size;
    }

    [[nodiscard]] T* Get(std::size_t index) noexcept
    {
        if (!data || index >= size)
            return nullptr;

        return &data[index];
    }

    [[nodiscard]] const T* Get(std::size_t index) const noexcept
    {
        if (!data || index >= size)
            return nullptr;

        return &data[index];
    }

    [[nodiscard]] T& operator[](std::size_t index) noexcept
    {
        return data[index];
    }

    [[nodiscard]] const T& operator[](std::size_t index) const noexcept
    {
        return data[index];
    }

    [[nodiscard]] T* begin() noexcept
    {
        return data;
    }

    [[nodiscard]] T* end() noexcept
    {
        return data ? data + size : nullptr;
    }

    [[nodiscard]] const T* begin() const noexcept
    {
        return data;
    }

    [[nodiscard]] const T* end() const noexcept
    {
        return data ? data + size : nullptr;
    }
};

static_assert(sizeof(void*) == 0x08);
static_assert(sizeof(GTA_At_Array_View<std::uint8_t>) == 0x10);
static_assert(alignof(GTA_At_Array_View<std::uint8_t>) == 0x08);
static_assert(offsetof(GTA_At_Array_View<std::uint8_t>, data) == 0x00);
static_assert(offsetof(GTA_At_Array_View<std::uint8_t>, size) == 0x08);
static_assert(offsetof(GTA_At_Array_View<std::uint8_t>, capacity) == 0x0A);
}
