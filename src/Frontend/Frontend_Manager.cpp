#include "Frontend_Manager.hpp"

#include "Backend/Logging/LoggerService.hpp"
#include "Frontend/Menu/Menu_Appearance.hpp"
#include "Frontend/Renderer/D3D12_Image_Loader.hpp"
#include "Frontend/Renderer/D3D12_Targets.hpp"
#include "Scripting/Lua/Lua_Runtime.hpp"

#include <chrono>
#include <utility>

namespace Devilz::Frontend
{
using namespace std::chrono_literals;

Frontend_Manager::~Frontend_Manager()
{
    Stop();
}

void Frontend_Manager::Start(
    const Integrations::GTA5_Enhanced::GTA_Module_Status& status,
    Backend::LoggerService& logger)
{
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true))
        return;

    Menu_Appearance_State::Instance().ConfigureLogger(&logger);

    m_bootstrapThread = std::jthread(
        [this, status, &logger](std::stop_token stopToken) mutable {
            Bootstrap(stopToken, std::move(status), &logger);
        });
}

void Frontend_Manager::Stop() noexcept
{
    if (!m_running.exchange(false) && !m_bootstrapThread.joinable())
        return;

    if (m_bootstrapThread.joinable()) {
        m_bootstrapThread.request_stop();
        m_bootstrapThread.join();
    }

    if (m_presentHook) {
        m_presentHook->Remove();
        m_presentHook.reset();
    }

    m_renderer.Shutdown();
    Renderer::D3D12_Image_Loader::Instance().Shutdown();
    Menu_Appearance_State::Instance().Shutdown();
    Scripting::Lua::Lua_Runtime::Instance().Shutdown();
}

void Frontend_Manager::Bootstrap(
    std::stop_token stopToken,
    Integrations::GTA5_Enhanced::GTA_Module_Status status,
    Backend::LoggerService* logger) noexcept
{
    if (!logger || !status.process || !status.build) {
        m_running.store(false);
        return;
    }

    Menu_Appearance_State::Instance().ConfigureLogger(logger);
    logger->Log(
        Backend::LogLevel::Info,
        "Frontend bootstrap waiting for GTA Enhanced D3D12 targets",
        "GTA5_Enhanced.Frontend");

    while (!stopToken.stop_requested() && m_running.load()) {
        auto targets = D3D12_Target_Resolver::Resolve(status);
        if (!targets) {
            std::this_thread::sleep_for(500ms);
            continue;
        }

        auto* swapChain = targets.Value().swapChain.Get();
        if (!m_renderer.Initialize(std::move(targets.Value()), *logger)) {
            logger->Log(
                Backend::LogLevel::Warning,
                "Frontend renderer initialization failed closed",
                "GTA5_Enhanced.Frontend");
            m_running.store(false);
            return;
        }

        m_presentHook = std::make_unique<Integrations::GTA5_Enhanced::D3D12_Present_Hook>(
            swapChain,
            m_renderer);

        auto installed = m_presentHook->Install();
        if (!installed) {
            logger->LogError(
                Backend::LogLevel::Warning,
                installed.Failure(),
                "GTA5_Enhanced.Frontend");
            m_presentHook.reset();
            m_renderer.Shutdown();
            Renderer::D3D12_Image_Loader::Instance().Shutdown();
            m_running.store(false);
            return;
        }

        logger->Log(
            Backend::LogLevel::Info,
            "Devils Den frontend ready | Menu: open | Toggle: INSERT | Appearance manager: enabled",
            "GTA5_Enhanced.Frontend");
        return;
    }
}
}
