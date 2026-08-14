#include "GTA_Script_Manager.hpp"

#include <Windows.h>

#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
GTA_Script_Manager& GTA_Script_Manager::Instance() noexcept
{
    static GTA_Script_Manager manager;
    return manager;
}

void GTA_Script_Manager::Reset() noexcept
{
    std::scoped_lock lock(m_mutex);
    m_scripts.clear();
    m_ready = false;
}

void GTA_Script_Manager::AddScript(std::string name, GTA_Script::Callback callback)
{
    std::scoped_lock lock(m_mutex);
    m_scripts.push_back(std::make_unique<GTA_Script>(std::move(name), std::move(callback)));
    m_ready = true;
}

void GTA_Script_Manager::Tick() noexcept
{
    if (!::IsThreadAFiber()) {
        if (!::ConvertThreadToFiber(nullptr))
            return;
    }

    std::scoped_lock lock(m_mutex);
    for (const auto& script : m_scripts) {
        if (script && !script->Done())
            script->Tick();
    }
}
}
