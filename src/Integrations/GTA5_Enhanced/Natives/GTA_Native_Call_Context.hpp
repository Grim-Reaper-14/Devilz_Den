#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct alignas(16) GTA_Native_Vector3
{
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

static_assert(sizeof(GTA_Native_Vector3) == 0x10);

struct GTA_Native_Call_Context
{
    void* returnValue = nullptr;                    // 0x00
    std::uint32_t argumentCount = 0;                // 0x08
    std::uint32_t pad0C = 0;                        // 0x0C
    void* arguments = nullptr;                      // 0x10
    std::int32_t vectorReferenceCount = 0;           // 0x18
    std::uint32_t pad1C = 0;                        // 0x1C
    GTA_Native_Vector3* vectorReferenceTargets[4]{}; // 0x20
    GTA_Native_Vector3 vectorReferenceSources[4]{}; // 0x40
};

static_assert(offsetof(GTA_Native_Call_Context, returnValue) == 0x00);
static_assert(offsetof(GTA_Native_Call_Context, argumentCount) == 0x08);
static_assert(offsetof(GTA_Native_Call_Context, arguments) == 0x10);
static_assert(offsetof(GTA_Native_Call_Context, vectorReferenceCount) == 0x18);
static_assert(offsetof(GTA_Native_Call_Context, vectorReferenceTargets) == 0x20);
static_assert(offsetof(GTA_Native_Call_Context, vectorReferenceSources) == 0x40);
static_assert(sizeof(GTA_Native_Call_Context) == 0x80);

class GTA_Native_Call_Frame final
{
public:
    static constexpr std::size_t MaxArguments = 40;
    static constexpr std::size_t ReturnSlots = 10;

    GTA_Native_Call_Frame() noexcept
    {
        m_context.returnValue = m_returns;
        m_context.arguments = m_arguments;
        Reset();
    }

    void Reset() noexcept
    {
        m_context.argumentCount = 0;
        m_context.vectorReferenceCount = 0;
        std::memset(m_returns, 0, sizeof(m_returns));
        std::memset(m_arguments, 0, sizeof(m_arguments));
    }

    template <typename T>
    [[nodiscard]] bool Push(T value) noexcept
    {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(sizeof(T) <= sizeof(std::uint64_t));

        if (m_context.argumentCount >= MaxArguments)
            return false;

        std::memcpy(&m_arguments[m_context.argumentCount], &value, sizeof(T));
        ++m_context.argumentCount;
        return true;
    }

    template <typename T>
    [[nodiscard]] T Return() const noexcept
    {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(sizeof(T) <= sizeof(std::uint64_t));

        T value{};
        std::memcpy(&value, m_returns, sizeof(T));
        return value;
    }

    [[nodiscard]] GTA_Native_Call_Context& Context() noexcept { return m_context; }
    [[nodiscard]] const GTA_Native_Call_Context& Context() const noexcept { return m_context; }

private:
    GTA_Native_Call_Context m_context{};
    std::uint64_t m_returns[ReturnSlots]{};
    std::uint64_t m_arguments[MaxArguments]{};
};
}
