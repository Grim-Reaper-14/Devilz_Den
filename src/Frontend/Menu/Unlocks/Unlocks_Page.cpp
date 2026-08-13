#include "Unlocks_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Stats_Extension.hpp"

#include <imgui.h>

#include <cstdint>
#include <cstring>
#include <string_view>

namespace Devilz::Frontend::Unlocks
{
namespace
{
void UnlockSection(const char* title, const char* detail)
{
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    ImGui::TextColored(palette.bronze, "%s", title);
    ImGui::TextWrapped("%s", detail);
    ImGui::Spacing();
}

void DrawStatEditor()
{
    using namespace Integrations::GTA5_Enhanced;

    auto& stats = GTA_Stats_State::Instance();
    const auto snapshot = stats.Snapshot();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    static char statName[128] = "MPX_";
    static char statValue[256]{};
    static std::uint64_t appliedRevision = 0;

    if (snapshot.revision != appliedRevision) {
        appliedRevision = snapshot.revision;
        if (!snapshot.value.empty() &&
            snapshot.status != GTA_Stat_Request_Status::Queued &&
            snapshot.status != GTA_Stat_Request_Status::NotFound &&
            snapshot.status != GTA_Stat_Request_Status::RuntimeUnavailable) {
            std::strncpy(statValue, snapshot.value.c_str(), sizeof(statValue) - 1);
            statValue[sizeof(statValue) - 1] = '\0';
        }
    }

    ImGui::TextColored(palette.bronze, "STAT EDITOR");
    ImGui::TextWrapped(
        "Reads are resolved from the live CStatsMgr table. MPX_ names are normalized to the active "
        "character. Supported writes are dispatched through GTA stat natives on the game thread.");

    ImGui::SetNextItemWidth(520.0F);
    ImGui::InputText("Stat", statName, sizeof(statName));

    const bool queued = snapshot.status == GTA_Stat_Request_Status::Queued;
    ImGui::BeginDisabled(queued || statName[0] == '\0');
    if (ImGui::Button("READ STAT", ImVec2(150.0F, 34.0F)))
        stats.RequestRead(statName);
    ImGui::EndDisabled();

    const bool sameRequest = snapshot.requestedName == std::string_view(statName);
    if (snapshot.typeKnown && sameRequest) {
        ImGui::SameLine();
        ImGui::TextDisabled(
            "%s | 0x%08X",
            GTAStatTypeName(snapshot.type),
            static_cast<unsigned int>(snapshot.hash));

        if (!snapshot.normalizedName.empty() && snapshot.normalizedName != snapshot.requestedName)
            ImGui::TextDisabled("Normalized: %s", snapshot.normalizedName.c_str());
    }

    ImGui::SetNextItemWidth(520.0F);
    ImGui::InputText("Value", statValue, sizeof(statValue));

    const bool canWrite = sameRequest && snapshot.typeKnown && snapshot.writeSupported &&
        !snapshot.controlledByNetShop && !queued;
    ImGui::BeginDisabled(!canWrite);
    if (ImGui::Button("WRITE STAT", ImVec2(150.0F, 34.0F)))
        stats.RequestWrite(statName, statValue);
    ImGui::EndDisabled();

    if (!sameRequest && statName[0] != '\0')
        ImGui::TextDisabled("Read this stat before writing it.");

    if (snapshot.serverAuthoritative && sameRequest) {
        ImGui::TextColored(
            palette.bronze,
            "Warning: this stat is server-authoritative; the server may reject or replace a client write.");
    }
    if (snapshot.controlledByNetShop && sameRequest) {
        ImGui::TextColored(
            palette.emberRed,
            "Write blocked: this stat is controlled by Netshop.");
    } else if (snapshot.typeKnown && sameRequest && !snapshot.writeSupported) {
        ImGui::TextDisabled("This stat type is read-only because no safe native write path is configured.");
    }

    if (snapshot.status != GTA_Stat_Request_Status::Idle && sameRequest) {
        ImGui::TextDisabled(
            "Status: %s%s%s",
            GTAStatRequestStatusName(snapshot.status),
            snapshot.detail.empty() ? "" : " - ",
            snapshot.detail.c_str());
    }
}
}

void DrawUnlocksPage()
{
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    ImGui::TextColored(palette.emberRed, "UNLOCKS");
    ImGui::SameLine();
    ImGui::TextDisabled("- progression, awards and content unlocks");
    Themes::Menu_Theme_Manager::Instance().DrawDivider();

    ImGui::TextWrapped(
        "Unlock tools live in this domain. Runtime actions are enabled only through verified GTA V Enhanced "
        "stat/native paths; category presets remain fail-closed until their individual mappings are verified.");

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    DrawStatEditor();

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    UnlockSection("PROGRESSION", "Rank, progression and character milestone unlocks.");
    UnlockSection("AWARDS", "Awards, achievements and challenge completion flags.");
    UnlockSection("CLOTHING", "Outfits, clothing items, masks and appearance unlocks.");
    UnlockSection("VEHICLES", "Vehicle availability, trade-price and related content unlocks.");
    UnlockSection("WEAPONS", "Weapon, component and equipment unlock flags.");
    UnlockSection("HEISTS", "Heist access, setup progression and associated unlock states.");
    UnlockSection("MISCELLANEOUS", "Additional verified unlockable content and progression flags.");

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    ImGui::TextDisabled("Preset unlock packs will appear here only after their stat mappings are verified.");
}
}
