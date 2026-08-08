#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Memory/Pattern_Manager.hpp"
#include "GTA_Patterns.hpp"
#include "GTA_Pointers.hpp"
#include "../Runtime/Build_Info.hpp"

#include <shared_mutex>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Pointer_Manager final
{
public:
    explicit GTA_Pointer_Manager(Devilz::Backend::Pattern_Manager& patterns) noexcept
        : m_patterns(patterns) {}

    Devilz::Backend::Result<GTA_Pointers> Resolve(const Build_Info& build,
                                                   std::string_view moduleName = "GTA5_Enhanced.exe");

    [[nodiscard]] GTA_Pointers Snapshot() const;
    [[nodiscard]] bool Ready() const noexcept;
    void Clear() noexcept;

private:
    Devilz::Backend::Result<void> Assign(const std::vector<Devilz::Backend::Pattern_Resolution>& resolutions,
                                         GTA_Pointers& pointers) const;
    Devilz::Backend::Result<void> Validate(const GTA_Pointers& pointers) const;

    Devilz::Backend::Pattern_Manager& m_patterns;
    mutable std::shared_mutex m_mutex;
    GTA_Pointers m_pointers{};
    std::uint64_t m_buildFingerprint = 0;
};
}
