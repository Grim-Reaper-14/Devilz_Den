#include "Unlocks_Page.hpp"

#include "Businesses_Unlock_Page.hpp"
#include "Clothing_Unlock_Page.hpp"
#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Stats_Extension.hpp"

#include <imgui.h>

#include <array>
#include <charconv>
#include <cstdio>
#include <string>
#include <string_view>

namespace Devilz::Frontend::Unlocks
{
namespace
{
using Integrations::GTA5_Enhanced::GTA_Stats_State;
using Integrations::GTA5_Enhanced::GTA_Stat_Type;

struct Stat_Editor_State
{
    std::array<char, 128> name{};
    std::array<char, 128> value{};
    GTA_Stat_Type type = GTA_Stat_Type::Int;
    bool useMpPrefix = true;
};

Stat_Editor_State& StatEditorState()
{
    static Stat_Editor_State state;
    return state;
}

bool ParseInt(std::string_view text, std::int32_t& value)
{
    if (text.empty())
        return false;
    const auto* first = text.data();
    const auto* last = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);
    return ec == std::errc{} && ptr == last;
}

void DrawStatEditor()
{
    auto& editor = StatEditorState();
    auto& stats = GTA_Stats_State::Instance();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    ImGui::TextColored(palette.bronze, "STAT EDITOR");
    ImGui::TextWrapped("Read and write supported GTA stats through the verified native runtime. MPX_ names automatically resolve to the active online character.");

    ImGui::SetNextItemWidth(430.0F);
    ImGui::InputTextWithHint("##StatName", "MPX_EXAMPLE_STAT or MPPLY_EXAMPLE", editor.name.data(), editor.name.size());

    const char* typeItems[] = {"INT", "BOOL"};
    int typeIndex = editor.type == GTA_Stat_Type::Bool ? 1 : 0;
    ImGui::SetNextItemWidth(180.0F);
    if (ImGui::Combo("TYPE", &typeIndex, typeItems, 2))
        editor.type = typeIndex == 1 ? GTA_Stat_Type::Bool : GTA_Stat_Type::Int;

    ImGui::SameLine();
    ImGui::Checkbox("AUTO MPX CHARACTER", &editor.useMpPrefix);

    ImGui::SetNextItemWidth(430.0F);
    ImGui::InputTextWithHint("##StatValue", editor.type == GTA_Stat_Type::Bool ? "true / false / 1 / 0" : "integer value", editor.value.data(), editor.value.size());

    const std::string statName(editor.name.data());
    if (ImGui::Button("READ STAT", ImVec2(140.0F, 34.0F))) {
        if (!statName.empty())
            stats.RequestRead(statName, editor.type, editor.useMpPrefix);
    }

    ImGui::SameLine();
    if (ImGui::Button("WRITE STAT", ImVec2(140.0F, 34.0F))) {
        if (!statName.empty()) {
            if (editor.type == GTA_Stat_Type::Bool) {
                const std::string_view text(editor.value.data());
                const bool value = text == "1" || text == "true" || text == "TRUE" || text == "True";
                stats.RequestWriteBool(statName, value, editor.useMpPrefix);
            } else {
                std::int32_t value = 0;
                if (ParseInt(editor.value.data(), value))
                    stats.RequestWriteInt(statName, value, editor.useMpPrefix);
            }
        }
    }

    const auto snapshot = stats.Snapshot();
    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    ImGui::TextColored(palette.bronze, "RUNTIME STATUS");
    ImGui::TextWrapped("%s", snapshot.detail.c_str());

    if (!snapshot.lastStatName.empty()) {
        ImGui::Text("Stat: %s", snapshot.lastStatName.c_str());
        if (snapshot.lastType == GTA_Stat_Type::Bool)
            ImGui::Text("Value: %s", snapshot.lastBool ? "true" : "false");
        else
            ImGui::Text("Value: %d", snapshot.lastInt);
    }
}
}

void DrawUnlocksPage()
{
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    ImGui::TextColored(palette.emberRed, "UNLOCKS");
    ImGui::TextWrapped("Unlock tooling is split by domain so each mapping can be validated independently against GTA V Enhanced.");

    if (!ImGui::BeginTabBar("##UnlockTabs"))
        return;

    if (ImGui::BeginTabItem("STAT EDITOR")) {
        DrawStatEditor();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("CLOTHING")) {
        Clothing::DrawClothingUnlocks();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("BUSINESSES")) {
        Businesses::DrawBusinessesUnlocks();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}
}
