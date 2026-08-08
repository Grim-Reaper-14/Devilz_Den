#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Filesystem/File_System_Manager.hpp"
#include "Native_Cache.hpp"
#include "Native_Crossmap.hpp"
#include "../Runtime/Build_Info.hpp"

#include <filesystem>
#include <optional>
#include <shared_mutex>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
class Native_Manager final
{
public:
    explicit Native_Manager(Devilz::Backend::File_System_Manager& files) noexcept;

    Devilz::Backend::Result<void> Initialize(
        Build_Info build,
        const std::filesystem::path& crossmapPath);
    void Shutdown() noexcept;

    [[nodiscard]] Devilz::Backend::Result<Native_Hash> Translate(Native_Hash canonical) const;
    [[nodiscard]] std::optional<Native_Registration> FindCached(Native_Hash canonical) const;
    void CacheResolved(Native_Hash canonical, Devilz::Backend::Pointer handler, bool validated);

    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] const Build_Info& Build() const noexcept { return m_build; }
    [[nodiscard]] Native_Crossmap& Crossmap() noexcept { return m_crossmap; }
    [[nodiscard]] Native_Cache& Cache() noexcept { return m_cache; }

private:
    Devilz::Backend::File_System_Manager& m_files;
    Build_Info m_build{};
    Native_Crossmap m_crossmap;
    Native_Cache m_cache;
    bool m_ready = false;
};
}
