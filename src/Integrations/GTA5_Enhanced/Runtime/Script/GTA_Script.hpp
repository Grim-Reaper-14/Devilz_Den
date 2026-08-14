#pragma once

#include <Windows.h>

#include <chrono>
#include <functional>
#include <optional>
#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Script final
{
public:
    using Clock = std::chrono::steady_clock;
    using Callback = std::function<void()>;

    GTA_Script(std::string name, Callback callback);
    ~GTA_Script();

    GTA_Script(const GTA_Script&) = delete;
    GTA_Script& operator=(const GTA_Script&) = delete;

    void Tick() noexcept;
    void YieldNow() noexcept;
    void YieldFor(std::chrono::milliseconds delay) noexcept;

    [[nodiscard]] bool Done() const noexcept { return m_done; }
    [[nodiscard]] const std::string& Name() const noexcept { return m_name; }

    [[nodiscard]] static GTA_Script* Current() noexcept;

private:
    static void WINAPI FiberEntry(void* parameter) noexcept;
    [[nodiscard]] bool EnsureFiber() noexcept;

    std::string m_name;
    Callback m_callback;
    void* m_childFiber = nullptr;
    void* m_mainFiber = nullptr;
    std::optional<Clock::time_point> m_wakeTime;
    bool m_done = false;
};
}
