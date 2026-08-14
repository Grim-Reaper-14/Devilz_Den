#pragma once

#include "GTA_Script.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Script_Manager final
{
public:
    static GTA_Script_Manager& Instance() noexcept;

    GTA_Script_Manager(const GTA_Script_Manager&) = delete;
    GTA_Script_Manager& operator=(const GTA_Script_Manager&) = delete;

    void Reset() noexcept;
    void AddScript(std::string name, GTA_Script::Callback callback);
    void Tick() noexcept;

    [[nodiscard]] bool Ready() const noexcept { return m_ready; }

private:
    GTA_Script_Manager() = default;

    bool m_ready = false;
    std::mutex m_mutex;
    std::vector<std::unique_ptr<GTA_Script>> m_scripts;
};
}
