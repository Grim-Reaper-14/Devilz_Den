#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Business_State.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstddef>

namespace Devilz::Frontend::Misc
{
namespace
{
using Integrations::GTA5_Enhanced::GTA_Business_State;
using Integrations::GTA5_Enhanced::GTA_Special_Cargo_Action_Kind;
using Integrations::GTA5_Enhanced::GTA_Special_Cargo_Action_Snapshot;
using Integrations::GTA5_Enhanced::GTA_Special_Cargo_Action_Status;
using Integrations::GTA5_Enhanced::GTA_Special_Cargo_Max_Held;
using Integrations::GTA5_Enhanced::GTA_Special_Cargo_Max_Item;
using Integrations::GTA5_Enhanced::GTA_Special_Cargo_Max_Sourcing_Amount;
using Integrations::GTA5_Enhanced::GTA_Special_Cargo_Max_Type;
using Integrations::GTA5_Enhanced::GTA_Special_Cargo_Min_Item;
using Integrations::GTA5_Enhanced::GTA_Special_Cargo_Min_Sourcing_Amount;
using Integrations::GTA5_Enhanced::GTA_Special_Cargo_Min_Type;
using Integrations::GTA5_Enhanced::GTA_Special_Cargo_Snapshot;
using Integrations::GTA5_Enhanced::GTA_Special_Cargo_Warehouse_Count;

struct SpecialCargoEditorState
{
    bool initialized = false;
    int playerIndex = -1;
    int sourcingAmount = GTA_Special_Cargo_Min_Sourcing_Amount;
    int cargoType = GTA_Special_Cargo_Min_Type;
    int specialItem = GTA_Special_Cargo_Min_Item;
    bool specialItemAvailable = false;
    std::array<int, GTA_Special_Cargo_Warehouse_Count> propertyIds{};
    std::array<int, GTA_Special_Cargo_Warehouse_Count> cargoHeld{};

    void Load(const GTA_Special_Cargo_Snapshot& snapshot)
    {
        initialized = true;
        playerIndex = snapshot.playerIndex;
        sourcingAmount = snapshot.sourcingAmount;
        cargoType = snapshot.cargoType;
        specialItem = snapshot.specialItem;
        specialItemAvailable = snapshot.specialItemAvailable;

        for (std::size_t slot = 0; slot < GTA_Special_Cargo_Warehouse_Count; ++slot) {
            propertyIds[slot] = snapshot.warehouses[slot].propertyId;
            cargoHeld[slot] = snapshot.warehouses[slot].cargoHeld;
        }
    }
};

SpecialCargoEditorState& SpecialCargoEditor()
{
    static SpecialCargoEditorState state;
    return state;
}

const char* SpecialCargoTypeName(int type) noexcept
{
    switch (type) {
    case -1: return "Any";
    case 0: return "Medical Supplies";
    case 1: return "Tobacco & Alcohol";
    case 2: return "Art & Antiques";
    case 3: return "Electronic Goods";
    case 4: return "Weapons & Ammo";
    case 5: return "Narcotics";
    case 6: return "Gemstones";
    case 7: return "Animal Materials";
    case 8: return "Counterfeit Goods";
    case 9: return "Jewelry";
    case 10: return "Bullion";
    default: return "Unknown";
    }
}

const char* SpecialCargoItemName(int item) noexcept
{
    switch (item) {
    case 0: return "Ornamental Egg";
    case 1: return "Golden Minigun";
    case 2: return "Extra Large Diamond";
    case 3: return "Sasquatch Hide";
    case 4: return "Film Reel";
    case 5: return "Rare Pocket Watch";
    default: return "Unknown";
    }
}

const char* SpecialCargoActionName(GTA_Special_Cargo_Action_Kind kind) noexcept
{
    switch (kind) {
    case GTA_Special_Cargo_Action_Kind::ApplySourcingSettings: return "Apply sourcing settings";
    case GTA_Special_Cargo_Action_Kind::SetWarehouseCargo: return "Set warehouse cargo";
    default: return "Special Cargo action";
    }
}

const char* SpecialCargoActionStatusName(GTA_Special_Cargo_Action_Status status) noexcept
{
    switch (status) {
    case GTA_Special_Cargo_Action_Status::Idle: return "IDLE";
    case GTA_Special_Cargo_Action_Status::Queued: return "QUEUED";
    case GTA_Special_Cargo_Action_Status::Succeeded: return "SUCCEEDED";
    case GTA_Special_Cargo_Action_Status::RuntimeUnavailable: return "RUNTIME UNAVAILABLE";
    case GTA_Special_Cargo_Action_Status::UnsupportedBuild: return "UNSUPPORTED BUILD";
    case GTA_Special_Cargo_Action_Status::NoWarehouse: return "NO WAREHOUSE";
    case GTA_Special_Cargo_Action_Status::InvalidValue: return "INVALID VALUE";
    case GTA_Special_Cargo_Action_Status::Failed: return "FAILED";
    default: return "UNKNOWN";
    }
}

void DrawSpecialCargoDivider()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void DrawSpecialCargoActionStatus(const GTA_Special_Cargo_Action_Snapshot& action)
{
    if (action.status == GTA_Special_Cargo_Action_Status::Idle)
        return;

    ImVec4 color{0.90F, 0.58F, 0.18F, 1.0F};
    if (action.status == GTA_Special_Cargo_Action_Status::Succeeded)
        color = ImVec4{0.42F, 0.78F, 0.42F, 1.0F};
    else if (action.status != GTA_Special_Cargo_Action_Status::Queued)
        color = ImVec4{0.92F, 0.32F, 0.28F, 1.0F};

    ImGui::TextColored(
        color,
        "[%s] %s",
        SpecialCargoActionStatusName(action.status),
        SpecialCargoActionName(action.kind));

    if (action.kind == GTA_Special_Cargo_Action_Kind::SetWarehouseCargo) {
        ImGui::SameLine();
        ImGui::TextDisabled("- warehouse %zu", action.warehouseSlot + 1U);
    }

    if (!action.detail.empty())
        ImGui::TextWrapped("%s", action.detail.c_str());
    ImGui::Spacing();
}

void DrawSpecialCargoControls()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();
    auto& business = GTA_Business_State::Instance();
    const auto cargo = business.SpecialCargo();
    const auto action = business.SpecialCargoAction();
    auto& editor = SpecialCargoEditor();

    bool layoutChanged = editor.playerIndex != cargo.playerIndex;
    if (!layoutChanged && editor.initialized) {
        for (std::size_t slot = 0; slot < GTA_Special_Cargo_Warehouse_Count; ++slot) {
            if (editor.propertyIds[slot] != cargo.warehouses[slot].propertyId) {
                layoutChanged = true;
                break;
            }
        }
    }

    if (cargo.runtimeReady && (!editor.initialized || layoutChanged))
        editor.Load(cargo);
    else if (!cargo.runtimeReady)
        editor.initialized = false;

    ImGui::TextColored(palette.bronze, "SPECIAL CARGO");
    DrawSpecialCargoDivider();
    ImGui::TextWrapped(
        "Enhanced 1.73 / b1158.13 Special Cargo sourcing controls and the five local-player "
        "warehouse slots. Writes are queued onto the GTA game thread and verified by readback.");
    ImGui::TextDisabled(
        "Sourcing: Global_1882762.f_13-f_16 | Warehouses: GPBD_FM property data f_128[i*3].");
    ImGui::Spacing();

    DrawSpecialCargoActionStatus(action);

    if (!cargo.runtimeReady) {
        ImGui::TextColored(
            ImVec4{0.90F, 0.58F, 0.18F, 1.0F},
            "Special Cargo runtime unavailable");
        ImGui::TextWrapped("%s", cargo.detail.c_str());
        return;
    }

    if (ImGui::Button("RELOAD LIVE VALUES", ImVec2{190.0F, 30.0F}))
        editor.Load(cargo);

    const bool actionQueued = action.status == GTA_Special_Cargo_Action_Status::Queued;
    if (actionQueued) {
        ImGui::SameLine();
        ImGui::TextDisabled("Waiting for the GTA game thread...");
    }

    DrawSpecialCargoDivider();
    ImGui::TextColored(palette.bronze, "SOURCING SETTINGS");
    ImGui::Spacing();

    ImGui::BeginDisabled(actionQueued || !editor.initialized);

    ImGui::SetNextItemWidth(320.0F);
    ImGui::SliderInt(
        "Sourcing Amount",
        &editor.sourcingAmount,
        GTA_Special_Cargo_Min_Sourcing_Amount,
        GTA_Special_Cargo_Max_Sourcing_Amount);

    ImGui::SetNextItemWidth(360.0F);
    if (ImGui::BeginCombo("Cargo Type (Lupe)", SpecialCargoTypeName(editor.cargoType))) {
        for (int type = GTA_Special_Cargo_Min_Type; type <= GTA_Special_Cargo_Max_Type; ++type) {
            const bool selected = editor.cargoType == type;
            ImGui::PushID(type + 100);
            if (ImGui::Selectable(SpecialCargoTypeName(type), selected))
                editor.cargoType = type;
            if (selected)
                ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }

    ImGui::SetNextItemWidth(360.0F);
    if (ImGui::BeginCombo("Special Item", SpecialCargoItemName(editor.specialItem))) {
        for (int item = GTA_Special_Cargo_Min_Item; item <= GTA_Special_Cargo_Max_Item; ++item) {
            const bool selected = editor.specialItem == item;
            ImGui::PushID(item + 200);
            if (ImGui::Selectable(SpecialCargoItemName(item), selected))
                editor.specialItem = item;
            if (selected)
                ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }

    ImGui::Checkbox("Special Item Available", &editor.specialItemAvailable);

    if (ImGui::Button("APPLY SOURCING SETTINGS", ImVec2{230.0F, 34.0F})) {
        editor.sourcingAmount = std::clamp(
            editor.sourcingAmount,
            GTA_Special_Cargo_Min_Sourcing_Amount,
            GTA_Special_Cargo_Max_Sourcing_Amount);
        editor.cargoType = std::clamp(
            editor.cargoType,
            GTA_Special_Cargo_Min_Type,
            GTA_Special_Cargo_Max_Type);
        editor.specialItem = std::clamp(
            editor.specialItem,
            GTA_Special_Cargo_Min_Item,
            GTA_Special_Cargo_Max_Item);

        (void)business.RequestSpecialCargoSourcingSettings(
            editor.sourcingAmount,
            editor.cargoType,
            editor.specialItem,
            editor.specialItemAvailable);
    }

    DrawSpecialCargoDivider();
    ImGui::TextColored(palette.bronze, "OWNED WAREHOUSES / CARGO HELD");
    ImGui::TextDisabled(
        "Slots are 1-5. Cargo writes target the +1 field after each property ID and are capped at 111.");
    ImGui::Spacing();

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("##SpecialCargoWarehouses", 5, tableFlags)) {
        ImGui::TableSetupColumn("SLOT");
        ImGui::TableSetupColumn("PROPERTY ID");
        ImGui::TableSetupColumn("LIVE CARGO");
        ImGui::TableSetupColumn("TARGET");
        ImGui::TableSetupColumn("ACTION");
        ImGui::TableHeadersRow();

        for (std::size_t slot = 0; slot < GTA_Special_Cargo_Warehouse_Count; ++slot) {
            const auto& warehouse = cargo.warehouses[slot];
            const bool owned = warehouse.available && warehouse.owned;

            ImGui::PushID(static_cast<int>(slot) + 500);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%zu", slot + 1U);

            ImGui::TableSetColumnIndex(1);
            if (warehouse.available)
                ImGui::Text("%d", warehouse.propertyId);
            else
                ImGui::TextDisabled("Unavailable");

            ImGui::TableSetColumnIndex(2);
            if (owned)
                ImGui::Text("%d", warehouse.cargoHeld);
            else
                ImGui::TextDisabled("Not owned");

            ImGui::TableSetColumnIndex(3);
            ImGui::BeginDisabled(!owned);
            ImGui::SetNextItemWidth(-1.0F);
            ImGui::InputInt("##SpecialCargoHeldTarget", &editor.cargoHeld[slot]);
            ImGui::EndDisabled();

            ImGui::TableSetColumnIndex(4);
            ImGui::BeginDisabled(!owned);
            if (ImGui::Button("SET CARGO", ImVec2{120.0F, 0.0F})) {
                editor.cargoHeld[slot] = std::clamp(
                    editor.cargoHeld[slot],
                    0,
                    GTA_Special_Cargo_Max_Held);
                (void)business.RequestSpecialCargoWarehouseCargo(
                    slot,
                    editor.cargoHeld[slot]);
            }
            ImGui::EndDisabled();
            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::TextDisabled(
        "The 111 ceiling is the supplied maximum. Smaller warehouse capacities are not inferred from this mapping.");
    ImGui::TextWrapped("%s", cargo.detail.c_str());
}
}
}
