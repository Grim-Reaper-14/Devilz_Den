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
    GTA_Stats_State& stats)
{
    if (!importer.active)
        return;

    if (importer.activeRequestId != 0) {
        const auto snapshot = stats.Snapshot(importer.activeRequestId);
        if (!snapshot) {
            ++importer.failed;
            importer.pending.clear();
            importer.activeRequestId = 0;
            importer.activeName.clear();
            importer.active = false;
            importer.total = importer.completed + importer.failed;
            importer.detail = "Clipboard import stopped because the stat runtime was reset.";
            return;
        }
        if (!IsFinished(snapshot->status))
            return;

        if (snapshot->status == GTA_Stat_Request_Status::Succeeded)
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
    importer.activeRequestId = stats.RequestWrite(
        std::move(operation.name),
        std::move(operation.value));
}

void DrawClipboardImporter(ClipboardImportState& importer)
{
    auto& stats = GTA_Stats_State::Instance();
    const auto metrics = stats.QueueSnapshot();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    ImGui::TextColored(palette.bronze, "LOAD NAME=VALUE PAIRS");
    ImGui::TextWrapped(
        "Paste one regular stat per line using STAT_NAME=value. Blank lines and lines beginning "
        "with //, #, or ; are ignored. MPX_ normalization is applied to every entry.");
    ImGui::Spacing();

    const bool queueFull = metrics.pending + metrics.inFlight >= metrics.capacity;
    ImGui::BeginDisabled(importer.active || queueFull);
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
                AdvanceClipboardImport(importer, stats);
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
        "Imports are serialized by request ID, so unrelated regular-stat requests cannot replace or confuse the active import result.");
}

void DrawQueueMonitor()
{
    auto& stats = GTA_Stats_State::Instance();
    const auto metrics = stats.QueueSnapshot();
    const auto history = stats.RecentHistory(32);
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    ImGui::TextColored(palette.bronze, "REGULAR STAT QUEUE");
    ImGui::TextWrapped(
        "FIFO transport for regular stat reads and writes. Up to %zu commands can wait or execute at once; "
        "the game-thread bridge drains a bounded number every gameplay tick.",
        metrics.capacity);
    ImGui::Spacing();

    ImGui::Text("Pending: %zu", metrics.pending);
    ImGui::SameLine();
    ImGui::TextDisabled("| In flight: %zu | Capacity: %zu", metrics.inFlight, metrics.capacity);
    ImGui::TextDisabled(
        "Completed %llu | Success %llu | Failed %llu | Cancelled %llu | Rejected %llu",
        static_cast<unsigned long long>(metrics.completed),
        static_cast<unsigned long long>(metrics.succeeded),
        static_cast<unsigned long long>(metrics.failed),
        static_cast<unsigned long long>(metrics.cancelled),
        static_cast<unsigned long long>(metrics.rejected));

    ImGui::BeginDisabled(metrics.pending == 0);
    if (ImGui::Button("CLEAR PENDING", ImVec2{170.0F, 34.0F}))
        (void)stats.ClearPending();
    ImGui::EndDisabled();

    DrawDivider();
    ImGui::TextColored(palette.bronze, "RECENT COMPLETIONS");
    if (history.empty()) {
        ImGui::TextDisabled("No completed regular-stat requests have run this session.");
        return;
    }

    constexpr auto flags =
        ImGuiTableFlags_BordersInnerH |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("##RegularStatQueueHistory", 4, flags, ImVec2{0.0F, 360.0F})) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 80.0F);
        ImGui::TableSetupColumn("STAT", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("STATUS", ImGuiTableColumnFlags_WidthFixed, 115.0F);
        ImGui::TableSetupColumn("VALUE", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const auto& request : history) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%llu", static_cast<unsigned long long>(request.requestId));
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(request.requestedName.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(StatusColor(request.status), "%s", GTAStatRequestStatusName(request.status));
            ImGui::TableSetColumnIndex(3);
            if (!request.value.empty())
                ImGui::TextUnformatted(request.value.c_str());
            else
                ImGui::TextDisabled("-");
        }

        ImGui::EndTable();
    }
}

std::string ScriptAbiText(const GTA_Script_Function_Descriptor& descriptor)
{
    std::string text = "(";
    for (std::size_t index = 0; index < descriptor.abi.arguments.size(); ++index) {
        if (index != 0)
            text += ", ";
        text += GTA_Script_Value_Type_Name(descriptor.abi.arguments[index]);
    }
    text += ") -> ";
