    text += GTA_Script_Value_Type_Name(descriptor.abi.returnType);
    return text;
}

void DrawScriptFunctionMonitor()
{
    using Integrations::GTA5_Enhanced::ClearScriptFunctionPcCache;
    using Integrations::GTA5_Enhanced::GTA_Script_Function_Exposure;
    using Integrations::GTA5_Enhanced::ScriptFunctionInvokerReady;
    using Integrations::GTA5_Enhanced::ScriptFunctionInvokeSnapshot;

    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    const auto metrics = ScriptFunctionInvokerMetrics();
    const auto latest = ScriptFunctionInvokeSnapshot();

    ImGui::TextColored(palette.bronze, "VERIFIED SCRIPT VM ENGINE");
    ImGui::TextWrapped(
        "Only registered or C++-verified descriptors can reach Rockstar ScriptVM. Build, script, "
        "signature, ABI, game-thread context, and stack safety are checked before every call.");
    ImGui::Spacing();

    ImGui::Text("Runtime: %s", ScriptFunctionInvokerReady() ? "READY" : "UNAVAILABLE");
    ImGui::SameLine();
    ImGui::TextDisabled(
        "| Registered: %zu | Pending: %zu | Cached: %zu",
        metrics.registeredDescriptors,
        metrics.pendingRequests,
        metrics.cachedDescriptors);
    ImGui::TextDisabled(
        "Executed %llu | Failed %llu | Rejected %llu | Cache hit/miss %llu/%llu | Scans %llu | Revalidations %llu",
        static_cast<unsigned long long>(metrics.invocationsExecuted),
        static_cast<unsigned long long>(metrics.invocationsFailed),
        static_cast<unsigned long long>(metrics.requestsRejected),
        static_cast<unsigned long long>(metrics.cacheHits),
        static_cast<unsigned long long>(metrics.cacheMisses),
        static_cast<unsigned long long>(metrics.patternScans),
        static_cast<unsigned long long>(metrics.signatureRevalidations));

    if (ImGui::Button("CLEAR RESOLVED PC CACHE", ImVec2{220.0F, 34.0F}))
        ClearScriptFunctionPcCache();

    DrawDivider();
    ImGui::TextColored(palette.bronze, "LATEST INVOCATION");
    if (latest.requestId == 0) {
        ImGui::TextDisabled("No Rockstar script function has been requested this session.");
    } else {
        ImGui::Text("#%llu  %s",
                    static_cast<unsigned long long>(latest.requestId),
                    GTA_Script_Function_Invoke_Status_Name(latest.status));
        ImGui::TextDisabled("%s | %s", latest.descriptorId.c_str(), latest.scriptName.c_str());
        ImGui::TextDisabled(
            "Signature PC 0x%X | Entry PC 0x%X | Cache %s | VM result %d",
            latest.signatureProgramCounter,
            latest.resolvedProgramCounter,
            latest.cacheHit ? "HIT" : "MISS",
            latest.scriptVmResult);
        if (latest.returnValue) {
            ImGui::TextDisabled(
                "Return %s | raw 0x%llX",
                GTA_Script_Value_Type_Name(latest.returnValue->type),
                static_cast<unsigned long long>(latest.returnValue->slot));
        }
        if (!latest.detail.empty())
            ImGui::TextWrapped("%s", latest.detail.c_str());
    }

    DrawDivider();
    ImGui::TextColored(palette.bronze, "REGISTERED DESCRIPTORS");
    const auto descriptors = RegisteredScriptFunctions(false);
    if (descriptors.empty()) {
        ImGui::TextDisabled("No verified function descriptors are registered yet.");
        return;
    }

    constexpr auto flags = ImGuiTableFlags_BordersInnerH |
                           ImGuiTableFlags_RowBg |
                           ImGuiTableFlags_ScrollY |
                           ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("##ScriptFunctionRegistry", 5, flags, ImVec2{0.0F, 360.0F})) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("SCRIPT", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("ABI", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("ACCESS", ImGuiTableColumnFlags_WidthFixed, 90.0F);
        ImGui::TableSetupColumn("SOURCE", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const auto& descriptor : descriptors) {
            const auto abi = ScriptAbiText(descriptor);
            const auto source = !descriptor.source.path.empty()
                ? descriptor.source.path
                : descriptor.sourceReference;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(descriptor.id.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(descriptor.scriptName.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(abi.c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::TextDisabled("%s", descriptor.exposure == GTA_Script_Function_Exposure::UserFacing ? "USER" : "INTERNAL");
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(source.c_str());
        }
        ImGui::EndTable();
    }
}
}

void DrawDebugPage()
{
    auto& stats = GTA_Stats_State::Instance();
    auto& importer = ClipboardState();
    AdvanceClipboardImport(importer, stats);
    const auto snapshot = stats.Snapshot();

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
            DrawClipboardImporter(importer);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("QUEUE")) {
            DrawQueueMonitor();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("SCRIPT VM")) {
            DrawScriptFunctionMonitor();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
}
