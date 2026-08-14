#include "Debug_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Packed_Stats_State.hpp"
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

    ImGui::BeginDisabled(!hasName || queued || clipboardImportActive);
    if (ImGui::Button("READ / REFRESH", ImVec2{160.0F, 34.0F}))
        stats.RequestRead(editor.name.data());
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
        !queued && !clipboardImportActive;
    ImGui::BeginDisabled(!canWrite);
    if (ImGui::Button("WRITE + VERIFY", ImVec2{160.0F, 34.0F}))
        stats.RequestWrite(editor.name.data(), editor.value.data());
    ImGui::EndDisabled();

    if (!sameRequest && hasName)
        ImGui::TextDisabled("Read this stat before writing it.");
    if (clipboardImportActive)
        ImGui::TextDisabled("Regular controls are paused while a clipboard batch is running.");

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
        if (!snapshot.detail.empty()) {
            ImGui::SameLine();
            ImGui::TextWrapped("%s", snapshot.detail.c_str());
        }
    }

    DrawDivider();
    ImGui::TextDisabled(
        "Verified Enhanced native paths support INT, FLOAT, BOOL, and STRING values and confirm writes by readback.");
}

void SynchronizePackedValue(PackedEditorState& editor)
{
    auto& packed = GTA_Packed_Stats_State::Instance();
    const auto batch = packed.BatchSnapshot();
    if (batch.revision == editor.appliedBatchRevision)
        return;

    editor.appliedBatchRevision = batch.revision;
    if (batch.active)
        return;

    const auto record = packed.Snapshot(editor.index);
    const auto selectedType = editor.type == 0
        ? GTA_Packed_Stat_Value_Type::Bool
        : GTA_Packed_Stat_Value_Type::Int;
    if (record.known && !record.failed && record.type == selectedType) {
        editor.boolValue = record.value;
        editor.intValue = record.intValue;
    }
}

void DrawPackedEditor()
{
    auto& packed = GTA_Packed_Stats_State::Instance();
    auto& editor = PackedState();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    SynchronizePackedValue(editor);
    const auto record = packed.Snapshot(editor.index);
    const auto batch = packed.BatchSnapshot();

    ImGui::TextColored(palette.bronze, "PACKED STAT");
    ImGui::TextWrapped(
        "Read or write one packed boolean or integer by index. Choose the type that owns the index; "
        "operations are queued onto the GTA game thread.");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(220.0F);
    ImGui::InputInt("Index", &editor.index);
    editor.index = (std::max)(editor.index, 0);

    constexpr const char* packedTypes[]{"BOOL", "INT"};
    ImGui::SetNextItemWidth(180.0F);
    ImGui::Combo("Type", &editor.type, packedTypes, 2);

    if (editor.type == 0)
        ImGui::Checkbox("Value", &editor.boolValue);
    else {
        ImGui::SetNextItemWidth(220.0F);
        ImGui::InputInt("Value", &editor.intValue);
    }

    ImGui::BeginDisabled(batch.active);
    if (ImGui::Button("READ PACKED", ImVec2{150.0F, 34.0F})) {
        if (editor.type == 0)
            (void)packed.RequestReads(std::vector<std::int32_t>{editor.index});
        else
            (void)packed.RequestIntReads(std::vector<std::int32_t>{editor.index});
    }
    ImGui::SameLine();
    if (ImGui::Button("WRITE PACKED", ImVec2{150.0F, 34.0F})) {
        if (editor.type == 0) {
            (void)packed.RequestWrites(
                std::vector<std::int32_t>{editor.index},
                editor.boolValue);
        } else {
            (void)packed.RequestIntWrites(
                std::vector<std::int32_t>{editor.index},
                editor.intValue);
        }
    }
    ImGui::EndDisabled();

    if (record.queued) {
        ImGui::TextColored(StatusColor(GTA_Stat_Request_Status::Queued), "[Queued]");
    } else if (record.failed) {
        ImGui::TextColored(StatusColor(GTA_Stat_Request_Status::Failed), "[Failed] Packed native call failed");
    } else if (record.known &&
        record.type == (editor.type == 0
            ? GTA_Packed_Stat_Value_Type::Bool
            : GTA_Packed_Stat_Value_Type::Int)) {
        if (record.type == GTA_Packed_Stat_Value_Type::Bool) {
            ImGui::TextColored(
                StatusColor(GTA_Stat_Request_Status::Succeeded),
                "[Known] %s",
                record.value ? "true" : "false");
        } else {
            ImGui::TextColored(
                StatusColor(GTA_Stat_Request_Status::Succeeded),
                "[Known] %d",
                record.intValue);
        }
    } else {
        ImGui::TextDisabled("This packed index has not been read yet.");
    }
}

void DrawPackedRangeEditor()
{
    auto& packed = GTA_Packed_Stats_State::Instance();
    auto& editor = PackedRangeState();
    const auto batch = packed.BatchSnapshot();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    ImGui::TextColored(palette.bronze, "PACKED RANGE");
    ImGui::TextWrapped(
        "Queue one boolean or integer value across an inclusive packed-stat range. The runtime caps a batch "
        "at %zu entries.",
        GTA_Packed_Stats_State::MaxQueuedCommands);
    ImGui::Spacing();

    ImGui::SetNextItemWidth(180.0F);
    ImGui::InputInt("Start", &editor.start);
    ImGui::SetNextItemWidth(180.0F);
    ImGui::InputInt("End", &editor.end);
    editor.start = (std::max)(editor.start, 0);
    editor.end = (std::max)(editor.end, 0);
    constexpr const char* packedTypes[]{"BOOL", "INT"};
    ImGui::SetNextItemWidth(180.0F);
    ImGui::Combo("Type", &editor.type, packedTypes, 2);
    if (editor.type == 0)
        ImGui::Checkbox("Range Value", &editor.boolValue);
    else {
        ImGui::SetNextItemWidth(220.0F);
        ImGui::InputInt("Range Value", &editor.intValue);
    }

    ImGui::BeginDisabled(batch.active);
    if (ImGui::Button("WRITE RANGE", ImVec2{160.0F, 34.0F})) {
        editor.detail.clear();
        if (editor.end < editor.start) {
            editor.detail = "End must be greater than or equal to Start.";
        } else {
            const auto count = static_cast<std::uint64_t>(editor.end) -
                static_cast<std::uint64_t>(editor.start) + 1U;
            if (count > GTA_Packed_Stats_State::MaxQueuedCommands) {
                editor.detail = "Range exceeds the packed-stat queue limit.";
            } else {
                std::vector<std::int32_t> indices;
                indices.reserve(static_cast<std::size_t>(count));
                for (std::uint64_t offset = 0; offset < count; ++offset) {
                    indices.push_back(static_cast<std::int32_t>(
                        static_cast<std::uint64_t>(editor.start) + offset));
                }
                editor.batchId = editor.type == 0
                    ? packed.RequestWrites(indices, editor.boolValue)
                    : packed.RequestIntWrites(indices, editor.intValue);
                editor.detail = "Packed range queued.";
            }
        }
    }
    ImGui::EndDisabled();

    if (editor.batchId != 0 && batch.batchId == editor.batchId) {
        const float progress = batch.total == 0
            ? 0.0F
            : static_cast<float>(batch.completed) / static_cast<float>(batch.total);
        ImGui::ProgressBar(progress, ImVec2{-1.0F, 0.0F});
        ImGui::TextDisabled(
            "%zu / %zu complete | %zu failed",
            batch.completed,
            batch.total,
            batch.failed);
        if (!batch.active && batch.completed == batch.total) {
            editor.detail = batch.failed == 0
                ? "Packed range completed."
                : "Packed range completed with failures.";
        }
    }

    if (!editor.detail.empty())
        ImGui::TextWrapped("%s", editor.detail.c_str());
}

[[nodiscard]] std::deque<ClipboardOperation> ParseClipboard(
    std::string_view text,
    std::size_t& malformed)
{
    std::deque<ClipboardOperation> operations;
    malformed = 0;

    while (!text.empty() && operations.size() < MaxClipboardOperations) {
        const auto newline = text.find('\n');
        const auto rawLine = newline == std::string_view::npos
            ? text
            : text.substr(0, newline);
        text = newline == std::string_view::npos
            ? std::string_view{}
            : text.substr(newline + 1);

        const auto line = Trim(rawLine);
        if (line.empty() || line.starts_with("//") ||
            line.front() == '#' || line.front() == ';') {
            continue;
        }

        const auto equals = line.find('=');
        if (equals == std::string_view::npos) {
            ++malformed;
            continue;
        }

        const auto name = Trim(line.substr(0, equals));
        const auto value = Trim(line.substr(equals + 1));
        if (name.empty() || value.empty()) {
            ++malformed;
            continue;
        }

        operations.push_back({std::string(name), std::string(value)});
    }

    return operations;
}

void AdvanceClipboardImport(
    ClipboardImportState& importer,
    GTA_Stats_State& stats,
    const GTA_Stat_Snapshot& snapshot)
{
    if (!importer.active)
        return;

    if (importer.activeRequestId != 0) {
        if (snapshot.requestId != importer.activeRequestId) {
            ++importer.failed;
            importer.pending.clear();
            importer.activeRequestId = 0;
            importer.activeName.clear();
            importer.active = false;
            importer.total = importer.completed + importer.failed;
            importer.detail = "Clipboard import stopped because the stat runtime was reset or replaced.";
            return;
        }
        if (!IsFinished(snapshot.status)) {
            return;
        }

        if (snapshot.status == GTA_Stat_Request_Status::Succeeded)
            ++importer.completed;
        else
            ++importer.failed;
        importer.activeRequestId = 0;
        importer.activeName.clear();
    }

    if (importer.pending.empty()) {
        importer.active = false;
        importer.detail = importer.failed == 0
            ? "Clipboard import completed."
            : "Clipboard import completed with failures.";
        return;
    }

    auto operation = std::move(importer.pending.front());
    importer.pending.pop_front();
    importer.activeName = operation.name;
    stats.RequestWrite(std::move(operation.name), std::move(operation.value));
    importer.activeRequestId = stats.Snapshot().requestId;
}

void DrawClipboardImporter(
    ClipboardImportState& importer,
    const GTA_Stat_Snapshot& snapshot)
{
    auto& stats = GTA_Stats_State::Instance();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    ImGui::TextColored(palette.bronze, "LOAD NAME=VALUE PAIRS");
    ImGui::TextWrapped(
        "Paste one regular stat per line using STAT_NAME=value. Blank lines and lines beginning "
        "with //, #, or ; are ignored. MPX_ normalization is applied to every entry.");
    ImGui::Spacing();

    const bool statBusy = snapshot.status == GTA_Stat_Request_Status::Queued;
    ImGui::BeginDisabled(importer.active || statBusy);
    if (ImGui::Button("LOAD FROM CLIPBOARD", ImVec2{210.0F, 34.0F})) {
        importer = {};
        const char* clipboard = ImGui::GetClipboardText();
        if (!clipboard || clipboard[0] == '\0') {
            importer.detail = "Clipboard is empty.";
        } else {
            importer.pending = ParseClipboard(clipboard, importer.malformed);
            importer.total = importer.pending.size();
            if (importer.pending.empty()) {
                importer.detail = "No valid STAT_NAME=value lines were found.";
            } else {
                importer.active = true;
                importer.detail = "Clipboard import started.";
                AdvanceClipboardImport(importer, stats, stats.Snapshot());
            }
        }
    }
    ImGui::EndDisabled();

    if (importer.active) {
        ImGui::SameLine();
        if (ImGui::Button("CANCEL AFTER CURRENT", ImVec2{190.0F, 34.0F})) {
            importer.pending.clear();
            importer.total = importer.completed + importer.failed +
                (importer.activeRequestId == 0 ? 0U : 1U);
            importer.detail = "Import cancellation requested.";
        }
    }

    if (importer.total != 0) {
        const auto finished = importer.completed + importer.failed;
        const float progress = static_cast<float>(finished) /
            static_cast<float>(importer.total);
        ImGui::ProgressBar(progress, ImVec2{-1.0F, 0.0F});
        ImGui::TextDisabled(
            "%zu / %zu complete | %zu failed | %zu malformed",
            finished,
            importer.total,
            importer.failed,
            importer.malformed);
        if (!importer.activeName.empty())
            ImGui::TextDisabled("Current: %s", importer.activeName.c_str());
    }

    if (!importer.detail.empty())
        ImGui::TextWrapped("%s", importer.detail.c_str());

    DrawDivider();
    ImGui::TextDisabled(
        "Imports are intentionally serialized so every write runs on the game thread and receives native readback verification.");
}
}

void DrawDebugPage()
{
    auto& stats = GTA_Stats_State::Instance();
    auto& importer = ClipboardState();
    auto snapshot = stats.Snapshot();
    AdvanceClipboardImport(importer, stats, snapshot);
    snapshot = stats.Snapshot();

    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    ImGui::TextColored(palette.emberRed, "DEBUG");
    ImGui::SameLine();
    ImGui::TextDisabled("- raw stat editor");

    DrawDivider();
    ImGui::TextColored(palette.emberRed, "ADVANCED TOOL");
    ImGui::SameLine();
    ImGui::TextWrapped(
        "Use exact, build-appropriate stat names and packed indices. Recovery remains the home for curated unlock presets.");
    DrawDivider();

    if (ImGui::BeginTabBar("##DebugStatEditorTabs")) {
        if (ImGui::BeginTabItem("REGULAR")) {
            DrawRegularEditor(snapshot, importer.active);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("PACKED")) {
            DrawPackedEditor();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("PACKED RANGE")) {
            DrawPackedRangeEditor();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("CLIPBOARD")) {
            DrawClipboardImporter(importer, snapshot);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
}
