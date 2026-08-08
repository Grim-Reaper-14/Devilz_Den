#pragma once

#include "Backend/Error/Result.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace Devilz::Integrations::GTA5_Enhanced
{
class Native_Call_Context final
{
public:
    static constexpr std::size_t MaxArguments = 32;
    static constexpr std::size_t MaxReturnSlots = 4;

    void Reset() noexcept
    {
        m_argumentCount = 0;
        m_returnCount = 0;
        m_arguments.fill(0);
        m_returns.fill(0);
    }

    template <typename T>
    Devilz::Backend::Result<void> Push(T value)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(sizeof(T) <= sizeof(std::uint64_t));
        if (m_argumentCount >= MaxArguments)
            return Devilz::Backend::Result<void>::Failure(
                Devilz::Backend::Error(Devilz::Backend::ErrorCode::RuntimeFailure,
                                       Devilz::Backend::ErrorCategory::Runtime,
                                       "Native call argument capacity exceeded"));

        std::uint64_t slot = 0;
        std::memcpy(&slot, &value, sizeof(T));
        m_arguments[m_argumentCount++] = slot;
        return Devilz::Backend::Result<void>::Success();
    }

    template <typename T>
    [[nodiscard]] T GetArgument(std::size_t index) const noexcept
    {
        T value{};
        if (index >= m_argumentCount) return value;
        std::memcpy(&value, &m_arguments[index], sizeof(T));
        return value;
    }

    template <typename T>
    void SetReturn(T value, std::size_t index = 0) noexcept
    {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(sizeof(T) <= sizeof(std::uint64_t));
        if (index >= MaxReturnSlots) return;
        std::uint64_t slot = 0;
        std::memcpy(&slot, &value, sizeof(T));
        m_returns[index] = slot;
        if (m_returnCount <= index) m_returnCount = index + 1;
    }

    template <typename T>
    [[nodiscard]] T GetReturn(std::size_t index = 0) const noexcept
    {
        T value{};
        if (index >= m_returnCount) return value;
        std::memcpy(&value, &m_returns[index], sizeof(T));
        return value;
    }

    [[nodiscard]] std::uint64_t* ArgumentData() noexcept { return m_arguments.data(); }
    [[nodiscard]] const std::uint64_t* ArgumentData() const noexcept { return m_arguments.data(); }
    [[nodiscard]] std::uint64_t* ReturnData() noexcept { return m_returns.data(); }
    [[nodiscard]] const std::uint64_t* ReturnData() const noexcept { return m_returns.data(); }
    [[nodiscard]] std::size_t ArgumentCount() const noexcept { return m_argumentCount; }
    [[nodiscard]] std::size_t ReturnCount() const noexcept { return m_returnCount; }

private:
    std::array<std::uint64_t, MaxArguments> m_arguments{};
    std::array<std::uint64_t, MaxReturnSlots> m_returns{};
    std::size_t m_argumentCount = 0;
    std::size_t m_returnCount = 0;
};
}
