#pragma once

#include "Frontend/Renderer/D3D12_Renderer.hpp"
#include "Integrations/GTA5_Enhanced/GTA_Module_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Hooks/D3D12_Present_Hook.hpp"

#include <atomic>
#include <memory>
#include <thread>

namespace Devilz::Backend
{
class LoggerService;
}

namespace Devilz::Frontend
{
class Frontend_Manager final
{
public:
    Frontend_Manager() = default;
    ~Frontend_Manager();

    Frontend_Manager(const Frontend_Manager&) = delete;
    Frontend_Manager& operator=(const Frontend_Manager&) = delete;

    void Start(
        const Integrations::GTA5_Enhanced::GTA_Module_Status& status,
        Backend::LoggerService& logger);
    void Stop() noexcept;

    [[nodiscard]] bool Running() const noexcept { return m_running.load(); }
    [[nodiscard]] bool Ready() const noexcept { return m_renderer.Ready(); }

private:
    void Bootstrap(
        std::stop_token stopToken,
        Integrations::GTA5_Enhanced::GTA_Module_Status status,
        Backend::LoggerService* logger) noexcept;

    D3D12_Renderer m_renderer;
    std::unique_ptr<Integrations::GTA5_Enhanced::D3D12_Present_Hook> m_presentHook;
    std::jthread m_bootstrapThread;
    std::atomic_bool m_running{false};
};
}
