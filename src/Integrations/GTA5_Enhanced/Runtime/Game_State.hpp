#pragma once
#include <atomic>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced {
enum class Game_State_Type : std::uint8_t { Unknown, Booting, Loading, Playing, Paused, Transitioning, ShuttingDown };
class Game_State final {
public:
    void Set(Game_State_Type state) noexcept { m_state.store(state, std::memory_order_release); }
    [[nodiscard]] Game_State_Type Get() const noexcept { return m_state.load(std::memory_order_acquire); }
    [[nodiscard]] bool IsPlaying() const noexcept { return Get() == Game_State_Type::Playing; }
private:
    std::atomic<Game_State_Type> m_state{Game_State_Type::Unknown};
};
}
