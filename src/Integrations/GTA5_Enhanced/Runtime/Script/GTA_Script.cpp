#include "GTA_Script.hpp"

#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
thread_local GTA_Script* g_currentScript = nullptr;
}

GTA_Script::GTA_Script(std::string name, Callback callback)
    : m_name(std::move(name)),
      m_callback(std::move(callback))
{
}

GTA_Script::~GTA_Script()
{
    if (m_childFiber)
        ::DeleteFiber(m_childFiber);
}

GTA_Script* GTA_Script::Current() noexcept
{
    return g_currentScript;
}

bool GTA_Script::EnsureFiber() noexcept
{
    if (m_childFiber)
        return true;

    m_childFiber = ::CreateFiber(0, &GTA_Script::FiberEntry, this);
    return m_childFiber != nullptr;
}

void GTA_Script::Tick() noexcept
{
    if (m_done || !EnsureFiber())
        return;

    const auto now = Clock::now();
    if (m_wakeTime && now < *m_wakeTime)
        return;

    m_mainFiber = ::GetCurrentFiber();
    g_currentScript = this;
    ::SwitchToFiber(m_childFiber);
    g_currentScript = nullptr;
}

void GTA_Script::Yield() noexcept
{
    m_wakeTime.reset();
    if (m_mainFiber)
        ::SwitchToFiber(m_mainFiber);
}

void GTA_Script::YieldFor(std::chrono::milliseconds delay) noexcept
{
    m_wakeTime = Clock::now() + delay;
    if (m_mainFiber)
        ::SwitchToFiber(m_mainFiber);
}

void WINAPI GTA_Script::FiberEntry(void* parameter) noexcept
{
    auto* script = static_cast<GTA_Script*>(parameter);
    if (!script)
        return;

    g_currentScript = script;
    try {
        if (script->m_callback)
            script->m_callback();
    } catch (...) {
    }

    script->m_done = true;
    g_currentScript = nullptr;
    if (script->m_mainFiber)
        ::SwitchToFiber(script->m_mainFiber);
}
}
