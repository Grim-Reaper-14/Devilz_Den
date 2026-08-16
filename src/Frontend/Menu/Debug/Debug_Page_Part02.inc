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

