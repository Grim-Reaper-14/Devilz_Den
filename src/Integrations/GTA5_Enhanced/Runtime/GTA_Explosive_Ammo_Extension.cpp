#include "GTA_Explosive_Ammo_Extension.hpp"

#include "GTA_Gameplay_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr GTA_Native_Hash IsPedArmedHash = 0x11552FA9DCB8E126ULL;
constexpr GTA_Native_Hash IsPedPerformingMeleeHash = 0xB73833BDAAE31047ULL;

int g_lastExplosionTimer = -1000;
GTA_Native_Script_Vector g_lastExplosionImpact{};

struct GTA_Script_Id_View
{
    void* vtable = nullptr;
    std::uint32_t hash = 0;
};

class GTA_Script_Handler_View
{
public:
    virtual ~GTA_Script_Handler_View() = default;
    virtual bool Unknown08() = 0;
    virtual void Unknown10() = 0;
    virtual void CleanupObjects() = 0;
    virtual GTA_Script_Id_View* Unknown20() = 0;
    virtual GTA_Script_Id_View* GetId() = 0;
};

// GTA V Enhanced layout mirrored from YimMenuV2's scrThread/GtaThread definitions:
// context script hash @ 0x10, scrThread script hash @ 0x150,
// script handler @ 0x198, and GtaThread script hash 2 @ 0x1A8.
struct GTA_Script_Thread_Spoof_View
{
    std::byte pad00[0x10]{};
    std::uint64_t contextScriptHash = 0;
    std::byte pad18[0x138]{};
    std::uint32_t scriptHash = 0;
    std::byte pad154[0x44]{};
    GTA_Script_Handler_View* scriptHandler = nullptr;
    void* netComponent = nullptr;
    std::uint32_t scriptHash2 = 0;
};

static_assert(offsetof(GTA_Script_Thread_Spoof_View, contextScriptHash) == 0x10);
static_assert(offsetof(GTA_Script_Thread_Spoof_View, scriptHash) == 0x150);
static_assert(offsetof(GTA_Script_Thread_Spoof_View, scriptHandler) == 0x198);
static_assert(offsetof(GTA_Script_Thread_Spoof_View, scriptHash2) == 0x1A8);
}

void TickExplosiveAmmoExtension(GTA_Native_Manager& natives, void* scriptThread) noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    if (!state.ExplosiveBullets() || !scriptThread)
        return;

    const auto ped = natives.Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return;

    const auto armed = natives.InvokeHash<bool>(IsPedArmedHash, *ped, 4);
    const auto melee = natives.InvokeHash<bool>(IsPedPerformingMeleeHash, *ped);
    if (!armed || !*armed || (melee && *melee))
        return;

    GTA_Native_Script_Vector impact{};
    const auto hit = natives.Invoke<bool>(GTA_Native_Id::GetPedLastWeaponImpactCoord, *ped, &impact);
    if (!hit || !*hit)
        return;

    const auto timer = natives.Invoke<int>(GTA_Native_Id::GetGameTimer);
    if (timer) {
        const bool sameImpact = std::fabs(impact.x - g_lastExplosionImpact.x) < 0.01F &&
            std::fabs(impact.y - g_lastExplosionImpact.y) < 0.01F &&
            std::fabs(impact.z - g_lastExplosionImpact.z) < 0.01F;
        if (sameImpact && *timer - g_lastExplosionTimer < 90)
            return;
        g_lastExplosionTimer = *timer;
        g_lastExplosionImpact = impact;
    }

    const auto orbitalHash = natives.Invoke<std::uint32_t>(GTA_Native_Id::GetHashKey, "am_mp_orbital_cannon");
    if (!orbitalHash)
        return;

    auto* thread = reinterpret_cast<GTA_Script_Thread_Spoof_View*>(scriptThread);
    const auto previousContextHash = thread->contextScriptHash;
    const auto previousHash = thread->scriptHash;
    const auto previousHash2 = thread->scriptHash2;

    GTA_Script_Id_View* scriptId = nullptr;
    std::uint32_t previousHandlerHash = 0;
    if (thread->scriptHandler) {
        scriptId = thread->scriptHandler->GetId();
        if (scriptId)
            previousHandlerHash = scriptId->hash;
    }

    thread->contextScriptHash = *orbitalHash;
    thread->scriptHash = *orbitalHash;
    thread->scriptHash2 = *orbitalHash;
    if (scriptId)
        scriptId->hash = *orbitalHash;

    (void)natives.Invoke<void>(GTA_Native_Id::AddOwnedExplosion,
        *ped,
        impact.x,
        impact.y,
        impact.z,
        state.ExplosionType(),
        state.ExplosionDamageScale(),
        true,
        false,
        state.ExplosionCameraShake());

    if (scriptId)
        scriptId->hash = previousHandlerHash;
    thread->scriptHash2 = previousHash2;
    thread->scriptHash = previousHash;
    thread->contextScriptHash = previousContextHash;
}
}
