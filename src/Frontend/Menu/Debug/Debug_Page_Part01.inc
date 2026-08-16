#include "Debug_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Packed_Stats_State.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Script_Function_Invoker.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Stats_Extension.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Devilz::Frontend::Debug
{
namespace
{
using Integrations::GTA5_Enhanced::GTA_Packed_Stats_State;
using Integrations::GTA5_Enhanced::GTA_Packed_Stat_Value_Type;
using Integrations::GTA5_Enhanced::GTA_Stat_Request_Status;
using Integrations::GTA5_Enhanced::GTA_Stat_Snapshot;
using Integrations::GTA5_Enhanced::GTA_Stats_State;
using Integrations::GTA5_Enhanced::GTA_Script_Function_Descriptor;
using Integrations::GTA5_Enhanced::GTA_Script_Function_Invoke_Status_Name;
using Integrations::GTA5_Enhanced::GTA_Script_Value_Type_Name;
using Integrations::GTA5_Enhanced::RegisteredScriptFunctions;
using Integrations::GTA5_Enhanced::ScriptFunctionInvokerMetrics;
using Integrations::GTA5_Enhanced::GTAStatRequestStatusName;
using Integrations::GTA5_Enhanced::GTAStatTypeName;

constexpr std::size_t MaxClipboardOperations = 512;

struct RegularEditorState
{
    std::array<char, 128> name{'M', 'P', 'X', '_'};
    std::array<char, 256> value{};
    std::uint64_t appliedRevision = 0;
};

struct PackedEditorState
{
    int index = 0;
    int type = 0;
    bool boolValue = false;
    int intValue = 0;
    std::uint64_t appliedBatchRevision = 0;
};

struct PackedRangeEditorState
{
    int start = 0;
    int end = 0;
    int type = 0;
    bool boolValue = true;
    int intValue = 0;
    std::uint64_t batchId = 0;
    std::string detail;
};

struct ClipboardOperation
{
    std::string name;
    std::string value;
};

struct ClipboardImportState
{
    std::deque<ClipboardOperation> pending;
    std::size_t total = 0;
    std::size_t completed = 0;
    std::size_t failed = 0;
    std::size_t malformed = 0;
    std::uint64_t activeRequestId = 0;
    std::string activeName;
    std::string detail;
    bool active = false;
};

RegularEditorState& RegularState()
{
    static RegularEditorState state;
    return state;
}

PackedEditorState& PackedState()
{
    static PackedEditorState state;
    return state;
}

PackedRangeEditorState& PackedRangeState()
{
    static PackedRangeEditorState state;
    return state;
}

ClipboardImportState& ClipboardState()
{
    static ClipboardImportState state;
    return state;
}

void DrawDivider()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

template <std::size_t Size>
void CopyToBuffer(std::string_view source, std::array<char, Size>& destination)
{
    static_assert(Size != 0);
    destination.fill('\0');
    const auto length = (std::min)(source.size(), Size - 1);
    if (length != 0)
        std::memcpy(destination.data(), source.data(), length);
}

[[nodiscard]] std::string_view Trim(std::string_view value)
{
    const auto isSpace = [](char character) {
        return character == ' ' || character == '\t' || character == '\r' || character == '\n';
    };

    while (!value.empty() && isSpace(value.front()))
        value.remove_prefix(1);
    while (!value.empty() && isSpace(value.back()))
        value.remove_suffix(1);
    return value;
}

[[nodiscard]] bool IsFinished(GTA_Stat_Request_Status status) noexcept
{
    return status != GTA_Stat_Request_Status::Idle &&
        status != GTA_Stat_Request_Status::Queued;
}

[[nodiscard]] ImVec4 StatusColor(GTA_Stat_Request_Status status) noexcept
{
    if (status == GTA_Stat_Request_Status::Succeeded)
        return {0.42F, 0.78F, 0.42F, 1.0F};
    if (status == GTA_Stat_Request_Status::Idle ||
        status == GTA_Stat_Request_Status::Queued)
        return {0.90F, 0.58F, 0.18F, 1.0F};
    return {0.92F, 0.32F, 0.28F, 1.0F};
}

void SynchronizeRegularValue(
    RegularEditorState& editor,
    const GTA_Stat_Snapshot& snapshot)
{
    if (snapshot.revision == editor.appliedRevision)
        return;

    editor.appliedRevision = snapshot.revision;
    if (snapshot.status == GTA_Stat_Request_Status::Succeeded &&
        snapshot.requestedName == editor.name.data()) {
        CopyToBuffer(snapshot.value, editor.value);
    }
}

void DrawRegularEditor(
    const GTA_Stat_Snapshot& snapshot,
    bool clipboardImportActive)
{
    auto& stats = GTA_Stats_State::Instance();
    auto& editor = RegularState();
    const auto metrics = stats.QueueSnapshot();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    SynchronizeRegularValue(editor, snapshot);

    ImGui::TextColored(palette.bronze, "REGULAR STAT");
    ImGui::TextWrapped(
        "Enter a stat name exactly as GTA defines it. MPX_ resolves to the active online "
        "character, and names without MP0_/MP1_ are retried with the active character prefix.");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputText("Stat Name", editor.name.data(), editor.name.size());

    const bool queued = snapshot.status == GTA_Stat_Request_Status::Queued;
    const bool hasName = editor.name[0] != '\0';
    const bool sameRequest = snapshot.requestedName == editor.name.data();
    const bool queueFull = metrics.pending + metrics.inFlight >= metrics.capacity;

    ImGui::BeginDisabled(!hasName || queued || clipboardImportActive || queueFull);
    if (ImGui::Button("READ / REFRESH", ImVec2{160.0F, 34.0F}))
        (void)stats.RequestRead(editor.name.data());
    ImGui::EndDisabled();

    if (snapshot.typeKnown && sameRequest) {
        ImGui::SameLine();
        ImGui::TextDisabled(
            "%s | 0x%08X",
            GTAStatTypeName(snapshot.type),
            static_cast<unsigned int>(snapshot.hash));

        if (!snapshot.normalizedName.empty() &&
            snapshot.normalizedName != snapshot.requestedName) {
            ImGui::TextDisabled("Normalized: %s", snapshot.normalizedName.c_str());
        }
    }

    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputText("Value", editor.value.data(), editor.value.size());

    const bool canWrite = sameRequest && snapshot.typeKnown &&
        snapshot.writeSupported && !snapshot.controlledByNetShop &&
        !queued && !clipboardImportActive && !queueFull;
    ImGui::BeginDisabled(!canWrite);
    if (ImGui::Button("WRITE + VERIFY", ImVec2{160.0F, 34.0F}))
        (void)stats.RequestWrite(editor.name.data(), editor.value.data());
    ImGui::EndDisabled();

    if (!sameRequest && hasName)
        ImGui::TextDisabled("Read this stat before writing it.");
    if (clipboardImportActive)
        ImGui::TextDisabled("Regular controls are paused while a clipboard batch is running.");
    if (queueFull)
        ImGui::TextDisabled("Regular stat queue is full; clear pending work or let the game thread drain it.");

    if (snapshot.serverAuthoritative && sameRequest) {
        ImGui::TextColored(
            palette.bronze,
            "Warning: GTA marks this stat server-authoritative; a later cloud sync may replace it.");
    }
    if (snapshot.controlledByNetShop && sameRequest) {
        ImGui::TextColored(
            palette.emberRed,
            "Write blocked: GTA marks this stat as Netshop-controlled.");
    } else if (snapshot.typeKnown && sameRequest && !snapshot.writeSupported) {
        ImGui::TextDisabled("This stat type has no verified write path in the current runtime.");
    }

    if (snapshot.status != GTA_Stat_Request_Status::Idle && sameRequest) {
        ImGui::TextColored(
            StatusColor(snapshot.status),
            "[%s]",
            GTAStatRequestStatusName(snapshot.status));
