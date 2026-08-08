#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Memory/Pointer.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace Devilz::Integrations::GTA5_Enhanced
{
class Script_Global_Manager;

class Script_Global final
{
public:
    Script_Global() noexcept = default;
    Script_Global(Script_Global_Manager* manager, std::uint32_t index) noexcept
        : m_manager(manager), m_index(index) {}

    [[nodiscard]] std::uint32_t Index() const noexcept { return m_index; }
    [[nodiscard]] Script_Global At(std::ptrdiff_t offset) const noexcept;
    [[nodiscard]] Devilz::Backend::Result<Devilz::Backend::Pointer> Resolve() const;

    template <typename T>
    [[nodiscard]] Devilz::Backend::Result<T> Read() const
    {
        static_assert(std::is_trivially_copyable_v<T>);
        auto pointer = Resolve();
        if (!pointer) return Devilz::Backend::Result<T>::Failure(pointer.Failure());
        T value{};
        std::memcpy(&value, reinterpret_cast<const void*>(pointer.Value().Address()), sizeof(T));
        return Devilz::Backend::Result<T>::Success(value);
    }

private:
    Script_Global_Manager* m_manager = nullptr;
    std::uint32_t m_index = 0;
};
}
