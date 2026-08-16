#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Business_State.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace Devilz::Frontend::Misc
{
namespace
{
using Integrations::GTA5_Enhanced::GTA_Business_State;
using Integrations::GTA5_Enhanced::GTA_Vehicle_Cargo_Action_Kind;
using Integrations::GTA5_Enhanced::GTA_Vehicle_Cargo_Action_Snapshot;
using Integrations::GTA5_Enhanced::GTA_Vehicle_Cargo_Action_Status;
using Integrations::GTA5_Enhanced::GTA_Vehicle_Cargo_Sell_Cooldown_Offsets;
using Integrations::GTA5_Enhanced::GTA_Vehicle_Cargo_Snapshot;
using Integrations::GTA5_Enhanced::GTA_Vehicle_Cargo_Steal_Cooldown_Offset;
using Integrations::GTA5_Enhanced::GTA_Vehicle_Cargo_Standard_Range_Value_Offset;
using Integrations::GTA5_Enhanced::GTA_Vehicle_Cargo_Mid_Range_Value_Offset;
using Integrations::GTA5_Enhanced::GTA_Vehicle_Cargo_Top_Range_Value_Offset;
using Integrations::GTA5_Enhanced::GTA_Vehicle_Cargo_Supplied_173_Values;
using Integrations::GTA5_Enhanced::GTA_Vehicle_Cargo_Values;

struct VehicleCargoEditorState
{
    bool initialized = false;
    GTA_Vehicle_Cargo_Values values = GTA_Vehicle_Cargo_Supplied_173_Values;

    void Load(const GTA_Vehicle_Cargo_Snapshot& snapshot)
    {
        initialized = true;
        values = snapshot.values;
    }

    void LoadSupplied173Values()
    {
        initialized = true;
        values = GTA_Vehicle_Cargo_Supplied_173_Values;
    }
};

VehicleCargoEditorState& VehicleCargoEditor()
{
    static VehicleCargoEditorState state;
    return state;
}

const char* VehicleCargoActionName(GTA_Vehicle_Cargo_Action_Kind kind) noexcept
{
    switch (kind) {
    case GTA_Vehicle_Cargo_Action_Kind::ApplyCooldowns: return "Apply cooldowns";
    case GTA_Vehicle_Cargo_Action_Kind::ApplySaleValues: return "Apply sale values";
    case GTA_Vehicle_Cargo_Action_Kind::ApplyAll: return "Apply all tunables";
    default: return "Vehicle Cargo action";
    }
}

const char* VehicleCargoActionStatusName(GTA_Vehicle_Cargo_Action_Status status) noexcept
{
    switch (status) {
    case GTA_Vehicle_Cargo_Action_Status::Idle: return "IDLE";
    case GTA_Vehicle_Cargo_Action_Status::Queued: return "QUEUED";
    case GTA_Vehicle_Cargo_Action_Status::Succeeded: return "SUCCEEDED";
    case GTA_Vehicle_Cargo_Action_Status::RuntimeUnavailable: return "RUNTIME UNAVAILABLE";
    case GTA_Vehicle_Cargo_Action_Status::UnsupportedBuild: return "UNSUPPORTED BUILD";
    case GTA_Vehicle_Cargo_Action_Status::InvalidValue: return "INVALID VALUE";
    case GTA_Vehicle_Cargo_Action_Status::Failed: return "FAILED";
    default: return "UNKNOWN";
    }
}

void DrawVehicleCargoDivider()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void DrawVehicleCargoActionStatus(const GTA_Vehicle_Cargo_Action_Snapshot& action)
{
    if (action.status == GTA_Vehicle_Cargo_Action_Status::Idle)
        return;

    ImVec4 color{0.90F, 0.58F, 0.18F, 1.0F};
    if (action.status == GTA_Vehicle_Cargo_Action_Status::Succeeded)
        color = ImVec4{0.42F, 0.78F, 0.42F, 1.0F};
    else if (action.status != GTA_Vehicle_Cargo_Action_Status::Queued)
        color = ImVec4{0.92F, 0.32F, 0.28F, 1.0F};

    ImGui::TextColored(
        color,
        "[%s] %s",
        VehicleCargoActionStatusName(action.status),
        VehicleCargoActionName(action.kind));

    if (!action.detail.empty())
        ImGui::TextWrapped("%s", action.detail.c_str());
    ImGui::Spacing();
}

void DrawVehicleCargoCooldownRow(
    const char* label,
    std::uint32_t offset,
    int liveValue,
    int& targetValue)
{
    ImGui::PushID(static_cast<int>(offset));
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("Global_262145.f_%u", offset);
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%d ms", liveValue);
    ImGui::TextDisabled("%.1f min", static_cast<float>(liveValue) / 60000.0F);
    ImGui::TableSetColumnIndex(3);
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputInt("##CooldownTarget", &targetValue, 60'000, 300'000);
    ImGui::PopID();
}

void DrawVehicleCargoSaleValueRow(
    const char* label,
    std::uint32_t offset,
    int liveValue,
    int& targetValue)
{
    ImGui::PushID(static_cast<int>(offset));
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("Global_262145.f_%u", offset);
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("$%d", liveValue);
    ImGui::TableSetColumnIndex(3);
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputInt("##SaleValueTarget", &targetValue, 5'000, 25'000);
    ImGui::PopID();
}

void ClampVehicleCargoValues(GTA_Vehicle_Cargo_Values& values) noexcept
{
    values.stealMissionCooldownMs = std::max(values.stealMissionCooldownMs, 0);
    for (auto& cooldown : values.sellMissionCooldownMs)
        cooldown = std::max(cooldown, 0);
    values.topRangeValue = std::max(values.topRangeValue, 0);
    values.midRangeValue = std::max(values.midRangeValue, 0);
    values.standardRangeValue = std::max(values.standardRangeValue, 0);
}

void DrawVehicleCargoControls()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();
    auto& business = GTA_Business_State::Instance();
    const auto cargo = business.VehicleCargo();
    const auto action = business.VehicleCargoAction();
    auto& editor = VehicleCargoEditor();

    if (cargo.runtimeReady && !editor.initialized)
        editor.Load(cargo);
    else if (!cargo.runtimeReady)
        editor.initialized = false;

    ImGui::TextColored(palette.bronze, "VEHICLE CARGO");
    DrawVehicleCargoDivider();
    ImGui::TextWrapped(
        "Enhanced 1.73 / b1158.13 Vehicle Cargo cooldown and sale-value tunables. "
        "Live values are read through the Business runtime; writes are queued onto the GTA game thread and verified by readback.");
    ImGui::TextDisabled(
        "Steal: f_19170 | Sell cooldowns: f_19525-f_19528 | Sale values: f_19263-f_19265");
    ImGui::Spacing();

    DrawVehicleCargoActionStatus(action);

    if (!cargo.runtimeReady) {
        ImGui::TextColored(
            ImVec4{0.90F, 0.58F, 0.18F, 1.0F},
            "Vehicle Cargo runtime unavailable");
        ImGui::TextWrapped("%s", cargo.detail.c_str());
        return;
    }

    if (ImGui::Button("RELOAD LIVE VALUES", ImVec2{190.0F, 30.0F}))
        editor.Load(cargo);
    ImGui::SameLine();
    if (ImGui::Button("LOAD SUPPLIED 1.73 VALUES", ImVec2{235.0F, 30.0F}))
        editor.LoadSupplied173Values();

    const bool actionQueued = action.status == GTA_Vehicle_Cargo_Action_Status::Queued;
    if (actionQueued) {
        ImGui::SameLine();
        ImGui::TextDisabled("Waiting for the GTA game thread...");
    }

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchProp;

    DrawVehicleCargoDivider();
    ImGui::TextColored(palette.bronze, "MISSION COOLDOWNS");
    ImGui::TextDisabled("Values are milliseconds. Set a target to 0 if you want no cooldown.");
    ImGui::Spacing();

    ImGui::BeginDisabled(actionQueued || !editor.initialized);
    if (ImGui::BeginTable("##VehicleCargoCooldowns", 4, tableFlags)) {
        ImGui::TableSetupColumn("SETTING");
        ImGui::TableSetupColumn("GLOBAL");
        ImGui::TableSetupColumn("LIVE");
        ImGui::TableSetupColumn("TARGET");
        ImGui::TableHeadersRow();

        DrawVehicleCargoCooldownRow(
            "Steal Mission",
            GTA_Vehicle_Cargo_Steal_Cooldown_Offset,
            cargo.values.stealMissionCooldownMs,
            editor.values.stealMissionCooldownMs);

        constexpr std::array<const char*, 4> sellLabels{{
            "Sell - 1 Vehicle",
            "Sell - 2 Vehicles",
            "Sell - 3 Vehicles",
            "Sell - 4 Vehicles",
        }};
        for (std::size_t index = 0; index < sellLabels.size(); ++index) {
            DrawVehicleCargoCooldownRow(
                sellLabels[index],
                GTA_Vehicle_Cargo_Sell_Cooldown_Offsets[index],
                cargo.values.sellMissionCooldownMs[index],
                editor.values.sellMissionCooldownMs[index]);
        }

        ImGui::EndTable();
    }

    if (ImGui::Button("APPLY COOLDOWNS", ImVec2{185.0F, 34.0F})) {
        ClampVehicleCargoValues(editor.values);
        (void)business.RequestVehicleCargoCooldowns(editor.values);
    }

    DrawVehicleCargoDivider();
    ImGui::TextColored(palette.bronze, "VEHICLE SALE VALUES");
    ImGui::Spacing();

    if (ImGui::BeginTable("##VehicleCargoSaleValues", 4, tableFlags)) {
        ImGui::TableSetupColumn("RANGE");
        ImGui::TableSetupColumn("GLOBAL");
        ImGui::TableSetupColumn("LIVE");
        ImGui::TableSetupColumn("TARGET");
        ImGui::TableHeadersRow();

        DrawVehicleCargoSaleValueRow(
            "Top Range",
            GTA_Vehicle_Cargo_Top_Range_Value_Offset,
            cargo.values.topRangeValue,
            editor.values.topRangeValue);
        DrawVehicleCargoSaleValueRow(
            "Mid Range",
            GTA_Vehicle_Cargo_Mid_Range_Value_Offset,
            cargo.values.midRangeValue,
            editor.values.midRangeValue);
        DrawVehicleCargoSaleValueRow(
            "Standard Range",
            GTA_Vehicle_Cargo_Standard_Range_Value_Offset,
            cargo.values.standardRangeValue,
            editor.values.standardRangeValue);

        ImGui::EndTable();
    }

    if (ImGui::Button("APPLY SALE VALUES", ImVec2{190.0F, 34.0F})) {
        ClampVehicleCargoValues(editor.values);
        (void)business.RequestVehicleCargoSaleValues(editor.values);
    }

    DrawVehicleCargoDivider();
    if (ImGui::Button("APPLY ALL VEHICLE CARGO", ImVec2{235.0F, 36.0F})) {
        ClampVehicleCargoValues(editor.values);
        (void)business.RequestVehicleCargoAll(editor.values);
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::TextDisabled("Opening this tab does not write any globals.");
    ImGui::Spacing();
    ImGui::TextWrapped("%s", cargo.detail.c_str());
}
}
}
