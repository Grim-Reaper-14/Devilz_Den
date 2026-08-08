#pragma once

#include "Backend/Memory/Pointer.hpp"

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Pointers
{
    Devilz::Backend::Pointer nativeTable{};
    Devilz::Backend::Pointer initNativeTables{};
    Devilz::Backend::Pointer gameState{};
    Devilz::Backend::Pointer scriptGlobals{};
    Devilz::Backend::Pointer scriptThreads{};
    Devilz::Backend::Pointer programTable{};
    Devilz::Backend::Pointer scriptVM{};
    Devilz::Backend::Pointer frameCount{};

    [[nodiscard]] bool CoreReady() const noexcept
    {
        return !gameState.IsNull() && !scriptGlobals.IsNull();
    }

    [[nodiscard]] bool NativeBootstrapReady() const noexcept
    {
        return !initNativeTables.IsNull();
    }

    [[nodiscard]] bool ScriptReady() const noexcept
    {
        return !scriptGlobals.IsNull() && !scriptThreads.IsNull() && !programTable.IsNull();
    }

    [[nodiscard]] bool Ready() const noexcept
    {
        return CoreReady() && ScriptReady();
    }
};
}
