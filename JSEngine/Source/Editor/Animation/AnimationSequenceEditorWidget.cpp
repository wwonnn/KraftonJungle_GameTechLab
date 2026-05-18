#include "Editor/Animation/AnimationSequenceEditorWidget.h"

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Editor/Animation/AnimationSequenceEditorDocument.h"
#include "Editor/Animation/AnimationSequenceEditorState.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"
#include "Editor/Animation/AnimationSequenceSequencerLayout.h"
#include "Editor/Animation/AnimationSequenceTimelineGeometry.h"
#include "Editor/Animation/AnimationSequenceViewerUtils.h"
#include "Editor/EditorEngine.h"
#include "Editor/Viewport/EditorViewportClient.h"
#include "Editor/Viewport/FSceneViewport.h"
#include "Engine/Asset/CurveFloatAsset.h"
#include "Engine/Runtime/WindowsWindow.h"
#include "Render/Common/RenderTypes.h"

#include "ImGui/imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

namespace
{
    constexpr float PreviewOverlayMargin = 10.0f;
    constexpr float PreviewOverlayPadding = 8.0f;
    constexpr float PreviewOverlayLineSpacing = 2.0f;

    constexpr float SequencerHeaderPadding = 8.0f;

    bool UsesAbsoluteImGuiCoordinates()
    {
        return (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
    }

    POINT ImGuiScreenToClientPoint(FWindowsWindow* Window, const ImVec2& Point)
    {
        POINT ClientPoint = { static_cast<LONG>(Point.x), static_cast<LONG>(Point.y) };
        if (Window && Window->GetHWND() && UsesAbsoluteImGuiCoordinates())
        {
            ::ScreenToClient(Window->GetHWND(), &ClientPoint);
        }

        return ClientPoint;
    }

    void SetOpaqueBlendStateCallback(const ImDrawList*, const ImDrawCmd* Cmd)
    {
        ID3D11DeviceContext* DeviceContext = static_cast<ID3D11DeviceContext*>(Cmd->UserCallbackData);
        if (!DeviceContext)
        {
            return;
        }

        const float BlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
        DeviceContext->OMSetBlendState(nullptr, BlendFactor, 0xffffffff);
    }

    float GetPreviewOverlayHeight(const TArray<FString>& Lines)
    {
        if (Lines.empty())
        {
            return 0.0f;
        }

        const float LineHeight = ImGui::GetTextLineHeight();
        return
            PreviewOverlayPadding * 2.0f +
            LineHeight * static_cast<float>(Lines.size()) +
            PreviewOverlayLineSpacing * static_cast<float>(std::max<int32>(0, static_cast<int32>(Lines.size()) - 1));
    }

    void DrawPreviewOverlay(const ImVec2& Min, const ImVec2& Max, const TArray<FString>& Lines)
    {
        if (Lines.empty())
        {
            return;
        }

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        const float AvailableWidth = std::max(1.0f, Max.x - Min.x - PreviewOverlayMargin * 2.0f);
        float MaxTextWidth = 0.0f;
        for (const FString& Line : Lines)
        {
            MaxTextWidth = std::max(MaxTextWidth, ImGui::CalcTextSize(Line.c_str()).x);
        }

        const float OverlayWidth = std::min(AvailableWidth, MaxTextWidth + PreviewOverlayPadding * 2.0f);
        const float OverlayHeight = GetPreviewOverlayHeight(Lines);
        const ImVec2 OverlayMin(Min.x + PreviewOverlayMargin, Min.y + PreviewOverlayMargin + 30.0f);
        const ImVec2 OverlayMax(OverlayMin.x + OverlayWidth, OverlayMin.y + OverlayHeight);

        DrawList->AddRectFilled(OverlayMin, OverlayMax, IM_COL32(16, 19, 25, 210), 6.0f);
        DrawList->AddRect(OverlayMin, OverlayMax, IM_COL32(255, 255, 255, 28), 6.0f);

        float TextY = OverlayMin.y + PreviewOverlayPadding;
        for (const FString& Line : Lines)
        {
            DrawList->AddText(
                ImVec2(OverlayMin.x + PreviewOverlayPadding, TextY),
                IM_COL32(230, 234, 242, 255),
                Line.c_str());
            TextY += ImGui::GetTextLineHeight() + PreviewOverlayLineSpacing;
        }
    }

    int32 GetCurveCount(const UAnimSequence* Sequence)
    {
        const UAnimDataModel* DataModel = AnimationSequenceViewer::GetValidAnimDataModel(Sequence);
        return DataModel ? static_cast<int32>(DataModel->CurveData.FloatCurves.size()) : 0;
    }

    const FFloatCurve* GetSelectedCurve(const UAnimSequence* Sequence, const FAnimationSequenceEditorState* State)
    {
        if (!State)
        {
            return nullptr;
        }

        const UAnimDataModel* DataModel = AnimationSequenceViewer::GetValidAnimDataModel(Sequence);
        if (!DataModel ||
            State->SelectedCurveIndex < 0 ||
            State->SelectedCurveIndex >= static_cast<int32>(DataModel->CurveData.FloatCurves.size()))
        {
            return nullptr;
        }

        return &DataModel->CurveData.FloatCurves[State->SelectedCurveIndex];
    }
}

void FAnimationSequenceEditorWidget::BindDocumentContext(
    FAnimationSequenceEditorDocument* InDocument,
    const FString& InSequencePath,
    UAnimSequence* InSequence,
    FAnimationSequencePreviewController* InPreviewController,
    FAnimationSequenceEditorState* InEditorState)
{
    Document = InDocument;
    SequencePath = InSequencePath;
    Sequence = InSequence;
    PreviewController = InPreviewController;
    EditorState = InEditorState;
}

void FAnimationSequenceEditorWidget::Render(float DeltaTime)
{
    (void)DeltaTime;

    const TArray<FString> PreviewOverlayLines = BuildPreviewOverlayLines();
    const float FooterHeight = ImGui::GetTextLineHeightWithSpacing() * 2.0f + 16.0f;
    const float AvailableHeight = std::max(ImGui::GetContentRegionAvail().y - FooterHeight, 300.0f);
    const float MinLayoutHeight =
        FAnimationSequenceSequencerLayout::PreviewMinHeight +
        FAnimationSequenceSequencerLayout::SequencerMinHeight +
        FAnimationSequenceSequencerLayout::SectionSplitterHeight;
    const float SplitLayoutHeight = std::max(AvailableHeight, MinLayoutHeight);
    const float MaxPreviewHeight =
        std::max(
            FAnimationSequenceSequencerLayout::PreviewMinHeight,
            SplitLayoutHeight -
            FAnimationSequenceSequencerLayout::SequencerMinHeight -
            FAnimationSequenceSequencerLayout::SectionSplitterHeight);

    if (EditorState && EditorState->PreviewPaneHeight <= 0.0f)
    {
        EditorState->PreviewPaneHeight = SplitLayoutHeight * 0.58f;
    }

    const float PreviewHeight = EditorState
        ? std::clamp(EditorState->PreviewPaneHeight, FAnimationSequenceSequencerLayout::PreviewMinHeight, MaxPreviewHeight)
        : SplitLayoutHeight * 0.58f;
    const float SequencerHeight = std::max(
        FAnimationSequenceSequencerLayout::SequencerMinHeight,
        SplitLayoutHeight - PreviewHeight - FAnimationSequenceSequencerLayout::SectionSplitterHeight);

    if (EditorState)
    {
        EditorState->PreviewPaneHeight = PreviewHeight;
    }

    RenderPreviewPane(PreviewHeight, PreviewOverlayLines);

    if (EditorState)
    {
        RenderSequencerRegion(SequencerHeight, MaxPreviewHeight);
    }

    ImGui::Spacing();
    RenderFooterStatus();
}

TArray<FString> FAnimationSequenceEditorWidget::BuildPreviewOverlayLines() const
{
    TArray<FString> PreviewOverlayLines;
    PreviewOverlayLines.push_back(
        AnimationSequenceViewer::IsLiveObject(Sequence) ? Sequence->GetName() : FString("Animation Sequence"));

    const UAnimDataModel* DataModel = AnimationSequenceViewer::GetValidAnimDataModel(Sequence);
    if (DataModel)
    {
        PreviewOverlayLines.push_back(
            std::to_string(DataModel->NumberOfFrames) +
            " frames  |  " +
            std::to_string(DataModel->NumberOfKeys) +
            " keys  |  " +
            std::to_string(static_cast<int32>(DataModel->BoneAnimationTracks.size())) +
            " bone tracks");
        PreviewOverlayLines.push_back(
            std::to_string(static_cast<int32>(DataModel->CurveData.FloatCurves.size())) +
            " curves  |  " +
            std::to_string(AnimationSequenceViewer::GetNotifyEventCount(Sequence)) +
            " notifies");
        PreviewOverlayLines.push_back(
            std::to_string(DataModel->FrameRate.Numerator) +
            " / " +
            std::to_string(DataModel->FrameRate.Denominator) +
            " fps");
    }
    else
    {
        PreviewOverlayLines.push_back("Sequence data model is unavailable.");
    }

    return PreviewOverlayLines;
}

void FAnimationSequenceEditorWidget::RenderPreviewPane(
    float PreviewHeight,
    const TArray<FString>& PreviewOverlayLines)
{
    ImVec2 PreviewSize(ImGui::GetContentRegionAvail().x, PreviewHeight);
    PreviewSize.x = std::max(PreviewSize.x, 1.0f);
    PreviewSize.y = std::max(PreviewSize.y, 1.0f);

    ImGui::BeginChild("AnimSequencePreviewViewport", PreviewSize, true, ImGuiWindowFlags_NoScrollbar);
    if (PreviewController)
    {
        PreviewController->SetViewportSize(static_cast<int32>(PreviewSize.x), static_cast<int32>(PreviewSize.y));
    }

    if (PreviewController && PreviewController->HasValidPreview() && PreviewController->GetPreviewSRV())
    {
        RenderPreviewImage(PreviewSize, PreviewOverlayLines);
    }
    else
    {
        RenderPreviewFallback(PreviewSize, PreviewOverlayLines);
    }

    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderPreviewImage(
    const ImVec2& PreviewSize,
    const TArray<FString>& PreviewOverlayLines)
{
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    const ImVec2 Min = ImGui::GetCursorScreenPos();
    const ImVec2 Max(Min.x + PreviewSize.x, Min.y + PreviewSize.y);

    ID3D11DeviceContext* DeviceContext =
        EditorEngine ? EditorEngine->GetRenderer().GetFD3DDevice().GetDeviceContext() : nullptr;
    DrawList->AddCallback(SetOpaqueBlendStateCallback, DeviceContext);
    DrawList->AddImage((ImTextureID)PreviewController->GetPreviewSRV(), Min, Max);
    DrawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

    ImGui::Dummy(PreviewSize);
    RenderPreviewToolbarOverlay(Min, Max);
    DrawPreviewOverlay(Min, Max, PreviewOverlayLines);

    const bool bViewportHovered = ImGui::IsItemHovered();
    const bool bViewportClicked =
        bViewportHovered &&
        (ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
            ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
            ImGui::IsMouseClicked(ImGuiMouseButton_Middle));

    SyncEmbeddedViewportRectAndFocus(Min, Max, bViewportClicked);
}

void FAnimationSequenceEditorWidget::RenderPreviewFallback(
    const ImVec2& PreviewSize,
    const TArray<FString>& PreviewOverlayLines) const
{
    const ImVec2 OverlayMin = ImGui::GetCursorScreenPos();
    const ImVec2 OverlayMax(OverlayMin.x + PreviewSize.x, OverlayMin.y + PreviewSize.y);
    ImGui::Dummy(ImVec2(0.0f, GetPreviewOverlayHeight(PreviewOverlayLines) + PreviewOverlayMargin * 2.0f));
    ImGui::TextWrapped(
        "%s",
        PreviewController ? PreviewController->GetPreviewStatusText().c_str() : "Preview controller is unavailable.");
    DrawPreviewOverlay(OverlayMin, OverlayMax, PreviewOverlayLines);
}

void FAnimationSequenceEditorWidget::RenderPreviewToolbarOverlay(const ImVec2& Min, const ImVec2& Max) const
{
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    const ImVec2 BarMin(Min.x + PreviewOverlayMargin, Min.y + PreviewOverlayMargin);
    const ImVec2 BarMax(Max.x - PreviewOverlayMargin, Min.y + PreviewOverlayMargin + 22.0f);
    DrawList->AddRectFilled(BarMin, BarMax, IM_COL32(12, 15, 20, 210), 6.0f);
    DrawList->AddRect(BarMin, BarMax, IM_COL32(255, 255, 255, 24), 6.0f);

    const FString SequenceName =
        AnimationSequenceViewer::IsLiveObject(Sequence) ? Sequence->GetName() : FString("Animation Sequence");
    DrawList->AddText(ImVec2(BarMin.x + 8.0f, BarMin.y + 4.0f), IM_COL32(233, 237, 244, 255), SequenceName.c_str());

    char StatusBuffer[128] = {};
    if (PreviewController)
    {
        snprintf(
            StatusBuffer,
            sizeof(StatusBuffer),
            "%s  |  %.3fs  |  %.2fx",
            PreviewController->IsPlaying() ? "Playing" : "Paused",
            PreviewController->GetCurrentTime(),
            PreviewController->GetPlayRate());
    }
    else
    {
        snprintf(StatusBuffer, sizeof(StatusBuffer), "Preview unavailable");
    }

    const ImVec2 StatusTextSize = ImGui::CalcTextSize(StatusBuffer);
    DrawList->AddText(
        ImVec2(BarMax.x - StatusTextSize.x - 10.0f, BarMin.y + 4.0f),
        IM_COL32(176, 183, 195, 255),
        StatusBuffer);
}

void FAnimationSequenceEditorWidget::SyncEmbeddedViewportRectAndFocus(
    const ImVec2& Min,
    const ImVec2& Max,
    bool bViewportClicked)
{
    if (!PreviewController)
    {
        return;
    }

    FSceneViewport* SceneViewport = PreviewController->GetSceneViewport();
    if (!SceneViewport)
    {
        return;
    }

    const POINT ClientMin = ImGuiScreenToClientPoint(EditorEngine ? EditorEngine->GetWindow() : nullptr, Min);
    FViewportRect NewRect = {};
    NewRect.X = static_cast<int32>(ClientMin.x);
    NewRect.Y = static_cast<int32>(ClientMin.y);
    NewRect.Width = static_cast<int32>(Max.x - Min.x);
    NewRect.Height = static_cast<int32>(Max.y - Min.y);

    // The scene viewport still receives renderer/input updates through the
    // editor viewport path, so the embedded ImGui image must keep its rect in
    // sync with the real viewport coordinates.
    SceneViewport->SetRect(NewRect);
    if (FEditorViewportClient* Client = SceneViewport->GetClient())
    {
        Client->SetViewportSize(static_cast<float>(NewRect.Width), static_cast<float>(NewRect.Height));
    }

    if (bViewportClicked && EditorEngine)
    {
        EditorEngine->FocusViewportInput(SceneViewport);
    }
}

void FAnimationSequenceEditorWidget::RenderSequencerRegion(float SequencerHeight, float MaxPreviewHeight)
{
    ImDrawList* SplitterDrawList = ImGui::GetWindowDrawList();
    const float SplitterWidth = std::max(ImGui::GetContentRegionAvail().x, 1.0f);
    ImGui::InvisibleButton(
        "##AnimSequencePreviewSequencerSplitter",
        ImVec2(SplitterWidth, FAnimationSequenceSequencerLayout::SectionSplitterHeight));
    const bool bSplitterHovered = ImGui::IsItemHovered();
    const bool bSplitterActive = ImGui::IsItemActive();
    const ImVec2 SplitterMin = ImGui::GetItemRectMin();
    const ImVec2 SplitterMax = ImGui::GetItemRectMax();
    const ImU32 SplitterColor = ImGui::GetColorU32(
        bSplitterActive ? ImGuiCol_SeparatorActive : (bSplitterHovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator));
    SplitterDrawList->AddRectFilled(SplitterMin, SplitterMax, SplitterColor, 2.0f);
    if (bSplitterHovered || bSplitterActive)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
    if (bSplitterActive)
    {
        EditorState->PreviewPaneHeight = std::clamp(
            EditorState->PreviewPaneHeight + ImGui::GetIO().MouseDelta.y,
            FAnimationSequenceSequencerLayout::PreviewMinHeight,
            MaxPreviewHeight);
    }

    ImGui::BeginChild("##AnimationSequenceSequencerRegion", ImVec2(0.0f, SequencerHeight), true);
    RenderTransportBar();

    const float DetailsPaneHeight =
        std::min(FAnimationSequenceSequencerLayout::DetailsPanelHeight, SequencerHeight * 0.34f);
    const float SplitPaneHeight = std::max(
        SequencerHeight -
        FAnimationSequenceSequencerLayout::TransportBarHeight -
        DetailsPaneHeight -
        SequencerHeaderPadding,
        140.0f);

    const float MaxOutlinerWidth = std::max(
        FAnimationSequenceSequencerLayout::OutlinerMinWidth,
        ImGui::GetContentRegionAvail().x - 280.0f);
    RenderSequencerSplitPane(SplitPaneHeight, DetailsPaneHeight, MaxOutlinerWidth);
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderTransportBar()
{
    ImGui::BeginChild(
        "##AnimationSequenceTransportBar",
        ImVec2(0.0f, FAnimationSequenceSequencerLayout::TransportBarHeight),
        true);
    ImGui::TextDisabled("Sequencer");

    const bool bCanTimelineControl =
        PreviewController != nullptr && EditorState != nullptr && EditorState->HasTimelineData();
    const bool bCanPlaybackControl = PreviewController != nullptr && PreviewController->HasValidPreview();

    RenderTransportControls(bCanTimelineControl, bCanPlaybackControl);
    RenderPlaybackSummary();
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderSequencerSplitPane(
    float SplitPaneHeight,
    float DetailsPaneHeight,
    float MaxOutlinerWidth)
{
    const float TotalWidth = std::max(ImGui::GetContentRegionAvail().x, 1.0f);
    EditorState->TrackOutlinerWidth = std::clamp(
        EditorState->TrackOutlinerWidth,
        FAnimationSequenceSequencerLayout::OutlinerMinWidth,
        MaxOutlinerWidth);

    const int32 NotifyTrackCount = Document ? Document->GetNotifyTrackCount() : 0;
    const int32 CurveCount = GetCurveCount(Sequence);
    const float TrackBodyHeight = FAnimationSequenceSequencerLayout::GetTrackBodyHeight(
        NotifyTrackCount,
        CurveCount,
        EditorState->bNotifiesExpanded,
        EditorState->bCurvesExpanded);
    const float TrackViewportHeight =
        SplitPaneHeight -
        FAnimationSequenceSequencerLayout::RulerHeight -
        FAnimationSequenceTimelineGeometry::VerticalPadding * 2.0f;
    const float MaxScrollY = std::max(
        0.0f,
        TrackBodyHeight + FAnimationSequenceSequencerLayout::TrackAreaPadding * 2.0f - TrackViewportHeight);
    EditorState->SequencerScrollY = std::clamp(EditorState->SequencerScrollY, 0.0f, MaxScrollY);

    ImGui::BeginChild("##AnimationSequenceSequencerSplit", ImVec2(0.0f, SplitPaneHeight), false, ImGuiWindowFlags_NoScrollbar);
    RenderTrackOutlinerPane(EditorState->TrackOutlinerWidth, SplitPaneHeight);

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::InvisibleButton(
        "##AnimationSequenceOutlinerSplitter",
        ImVec2(FAnimationSequenceSequencerLayout::OutlinerSplitterWidth, SplitPaneHeight));
    const bool bHovered = ImGui::IsItemHovered();
    const bool bActive = ImGui::IsItemActive();
    const ImVec2 SplitterMin = ImGui::GetItemRectMin();
    const ImVec2 SplitterMax = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddRectFilled(
        SplitterMin,
        SplitterMax,
        ImGui::GetColorU32(bActive ? ImGuiCol_SeparatorActive : (bHovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator)),
        2.0f);
    if (bHovered || bActive)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    if (bActive)
    {
        EditorState->TrackOutlinerWidth = std::clamp(
            EditorState->TrackOutlinerWidth + ImGui::GetIO().MouseDelta.x,
            FAnimationSequenceSequencerLayout::OutlinerMinWidth,
            MaxOutlinerWidth);
    }

    ImGui::SameLine(0.0f, 0.0f);
    RenderTimelineCanvasPane(
        std::max(ImGui::GetContentRegionAvail().x, 1.0f),
        SplitPaneHeight);

    ImGui::EndChild();
    RenderSelectionDetailsPane(DetailsPaneHeight);
}

void FAnimationSequenceEditorWidget::RenderTrackOutlinerPane(float Width, float Height)
{
    ImGui::BeginChild(
        "##AnimationSequenceTrackOutlinerPane",
        ImVec2(Width, Height),
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    TrackOutlinerWidget.Render(*EditorState, Document, Sequence);
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
        std::fabs(ImGui::GetIO().MouseWheel) > 0.0f)
    {
        EditorState->SequencerScrollY = std::max(EditorState->SequencerScrollY - ImGui::GetIO().MouseWheel * 24.0f, 0.0f);
    }
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderTimelineCanvasPane(float Width, float Height)
{
    ImGui::BeginChild(
        "##AnimationSequenceTimelineCanvasPane",
        ImVec2(Width, Height),
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const int32 NotifyTrackCount = Document ? Document->GetNotifyTrackCount() : 0;
    const int32 CurveCount = GetCurveCount(Sequence);
    const float RequiredCanvasHeight =
        FAnimationSequenceTimelineGeometry::VerticalPadding * 2.0f +
        TimelineWidget.GetRulerHeight() +
        FAnimationSequenceSequencerLayout::TrackAreaPadding * 2.0f +
        FAnimationSequenceSequencerLayout::GetTrackBodyHeight(
            NotifyTrackCount,
            CurveCount,
            EditorState->bNotifiesExpanded,
            EditorState->bCurvesExpanded);

    const ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
    const ImVec2 CanvasSize(
        std::max(1.0f, ImGui::GetContentRegionAvail().x),
        std::max(std::max(Height - 2.0f, 1.0f), RequiredCanvasHeight));
    ImGui::InvisibleButton("##AnimationSequenceSequencerCanvas", CanvasSize);

    const bool bCanvasHovered = ImGui::IsItemHovered();
    const bool bCanvasActive = ImGui::IsItemActive();
    const FAnimationSequenceTimelineGeometry Geometry =
        FAnimationSequenceTimelineGeometry::BuildSequencerCanvasGeometry(CanvasPos, CanvasSize, TimelineWidget.GetRulerHeight());

    TimelineWidget.RenderCanvas(*EditorState, PreviewController, Geometry, bCanvasHovered, bCanvasActive);

    const float NotifySectionTop =
        Geometry.TrackTop +
        FAnimationSequenceSequencerLayout::TrackAreaPadding -
        EditorState->SequencerScrollY;
    NotifyLaneWidget.RenderRows(*EditorState, Document, Geometry, NotifySectionTop);

    const float CurveSectionTop =
        NotifySectionTop +
        FAnimationSequenceSequencerLayout::GetNotifySectionHeight(NotifyTrackCount, EditorState->bNotifiesExpanded) +
        FAnimationSequenceSequencerLayout::SectionGap;
    CurveTrackWidget.RenderRows(Sequence, *EditorState, Geometry, CurveSectionTop);
    if (Document &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        EditorState->HoveredCurveIndex >= 0)
    {
        Document->ClearNotifySelection();
    }

    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderSelectionDetailsPane(float Height)
{
    ImGui::BeginChild("##AnimationSequenceSelectionDetails", ImVec2(0.0f, Height), true);
    ImGui::TextDisabled("Selection & Activity");
    if (ImGui::BeginTable("##AnimationSequenceSelectionDetailsTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Selection", ImGuiTableColumnFlags_WidthStretch, 0.55f);
        ImGui::TableSetupColumn("Activity", ImGuiTableColumnFlags_WidthStretch, 0.45f);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        RenderNotifyDetailsPanel();
        RenderCurveInspectionPanel();

        ImGui::TableSetColumnIndex(1);
        RenderRecentNotifySummary();
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderTransportControls(bool bCanTimelineControl, bool bCanPlaybackControl)
{
    if (!bCanTimelineControl)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("|<") && PreviewController)
    {
        PreviewController->JumpToStart();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }
    ImGui::SameLine();
    if (ImGui::Button("< Frame") && PreviewController)
    {
        PreviewController->StepToPreviousFrame();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }
    ImGui::SameLine();

    if (!bCanPlaybackControl)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Play") && PreviewController)
    {
        PreviewController->Play();
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause") && PreviewController)
    {
        PreviewController->Pause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop") && PreviewController)
    {
        PreviewController->Stop();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }

    if (!bCanPlaybackControl)
    {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Frame >") && PreviewController)
    {
        PreviewController->StepToNextFrame();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }
    ImGui::SameLine();
    if (ImGui::Button(">|") && PreviewController)
    {
        PreviewController->JumpToEnd();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }
    ImGui::SameLine(0.0f, 18.0f);

    bool bLooping = EditorState->bLoop;
    if (ImGui::Checkbox("Loop", &bLooping) && PreviewController)
    {
        PreviewController->SetLooping(bLooping);
        EditorState->bLoop = bLooping;
    }

    ImGui::SameLine();
    float PlaybackRate = EditorState->PlayRate;
    ImGui::SetNextItemWidth(130.0f);
    if (ImGui::DragFloat("Play Rate", &PlaybackRate, 0.05f, -4.0f, 4.0f, "%.2fx") && PreviewController)
    {
        PreviewController->SetPlayRate(PlaybackRate);
        EditorState->PlayRate = PreviewController->GetPlayRate();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Snap", &EditorState->bSnapToFrames);

    if (!bCanTimelineControl)
    {
        ImGui::EndDisabled();
    }
}

void FAnimationSequenceEditorWidget::RenderPlaybackSummary() const
{
    const int32 CurrentFrame = EditorState->TimeToFrameIndex(EditorState->CurrentTime);
    const int32 TotalFrames = std::max(EditorState->TotalFrames, 0);
    ImGui::Spacing();
    ImGui::Text(
        "Time %.3fs / %.3fs    Frame %d / %d    Rate %.2f fps    Playback %s",
        EditorState->CurrentTime,
        EditorState->SequenceLength,
        TotalFrames > 0 ? CurrentFrame + 1 : 0,
        TotalFrames,
        EditorState->GetDisplayFPS(),
        PreviewController && PreviewController->IsPlaying() ? "Playing" : "Paused");
}

void FAnimationSequenceEditorWidget::RenderFooterStatus() const
{
    ImGui::TextDisabled(
        "%s",
        PreviewController ? PreviewController->GetTimelineStatusText().c_str() : "Timeline is unavailable.");

    if (!(PreviewController && PreviewController->HasSequence()))
    {
        ImGui::TextDisabled("Animation sequence is unavailable.");
    }
}

void FAnimationSequenceEditorWidget::RenderNotifyDetailsPanel()
{
    ImGui::TextDisabled("Selected Notify");

    if (!Document || !EditorState)
    {
        ImGui::TextDisabled("Notify editing is unavailable.");
        return;
    }

    const int32 DefaultTrackIndex = std::max(EditorState->SelectedNotifyTrackIndex, 0);
    if (ImGui::Button("Add Notify"))
    {
        Document->AddNotifyAtTime(DefaultTrackIndex, EditorState->CurrentTime);
    }

    const bool bHasSelection = Document->GetSelectedNotify() != nullptr;
    ImGui::SameLine();
    if (!bHasSelection)
    {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Delete Selected"))
    {
        Document->DeleteSelectedNotify();
    }
    if (!bHasSelection)
    {
        ImGui::EndDisabled();
    }

    const FAnimNotifyEvent* SelectedNotify = Document->GetSelectedNotify();
    if (!SelectedNotify)
    {
        ImGui::TextDisabled("Select a notify marker to edit it.");
        return;
    }

    const FAnimNotifyEvent NotifySnapshot = *SelectedNotify;

    char NameBuffer[256] = {};
    const FString NotifyName = NotifySnapshot.Name.IsValid() ? NotifySnapshot.Name.ToString() : FString();
    strncpy_s(NameBuffer, sizeof(NameBuffer), NotifyName.c_str(), _TRUNCATE);
    if (ImGui::InputText("Name", NameBuffer, sizeof(NameBuffer)))
    {
        Document->SetSelectedNotifyName(FName(NameBuffer));
    }

    float NotifyTime = NotifySnapshot.Time;
    const float MaxTime = std::max(EditorState->SequenceLength, 0.0f);
    if (ImGui::DragFloat("Time", &NotifyTime, 0.001f, 0.0f, MaxTime, "%.3f s"))
    {
        Document->SetSelectedNotifyTime(NotifyTime, EditorState->bSnapToFrames);
    }

    float NotifyDuration = NotifySnapshot.Duration;
    if (ImGui::DragFloat("Duration", &NotifyDuration, 0.001f, 0.0f, MaxTime, "%.3f s"))
    {
        Document->SetSelectedNotifyDuration(NotifyDuration);
    }

    float Color[4] =
    {
        NotifySnapshot.Color.R,
        NotifySnapshot.Color.G,
        NotifySnapshot.Color.B,
        NotifySnapshot.Color.A
    };
    if (ImGui::ColorEdit4("Color", Color))
    {
        Document->SetSelectedNotifyColor(FColor(
            std::clamp(Color[0], 0.0f, 1.0f),
            std::clamp(Color[1], 0.0f, 1.0f),
            std::clamp(Color[2], 0.0f, 1.0f),
            std::clamp(Color[3], 0.0f, 1.0f)));
    }

    ImGui::TextDisabled(
        "Track %d  StableId %s",
        EditorState->SelectedNotifyTrackIndex + 1,
        NotifySnapshot.StableId.ToString().c_str());
}

void FAnimationSequenceEditorWidget::RenderCurveInspectionPanel() const
{
    ImGui::Spacing();
    ImGui::TextDisabled("Selected Curve");

    const UAnimDataModel* DataModel = AnimationSequenceViewer::GetValidAnimDataModel(Sequence);
    if (!DataModel || DataModel->CurveData.FloatCurves.empty())
    {
        ImGui::TextDisabled("No float curves in this sequence.");
        return;
    }

    const FFloatCurve* SelectedCurve = GetSelectedCurve(Sequence, EditorState);
    if (!SelectedCurve)
    {
        ImGui::TextDisabled(
            "%d curves available. Select a curve from the outliner or graph row.",
            static_cast<int32>(DataModel->CurveData.FloatCurves.size()));
        return;
    }

    float MinTime = 0.0f;
    float MaxTime = 0.0f;
    if (!SelectedCurve->Keys.empty())
    {
        MinTime = SelectedCurve->Keys.front().Time;
        MaxTime = SelectedCurve->Keys.front().Time;
        for (const FCurveKey& Key : SelectedCurve->Keys)
        {
            MinTime = std::min(MinTime, Key.Time);
            MaxTime = std::max(MaxTime, Key.Time);
        }
    }

    ImGui::BulletText("%s", SelectedCurve->CurveName.ToString().c_str());
    ImGui::BulletText("%d keys", static_cast<int32>(SelectedCurve->Keys.size()));
    ImGui::BulletText("Range %.3fs - %.3fs", MinTime, MaxTime);
}

void FAnimationSequenceEditorWidget::RenderRecentNotifySummary() const
{
    ImGui::TextDisabled("Recent Fired Notifies");

    if (!PreviewController)
    {
        ImGui::TextDisabled("Preview controller is unavailable.");
        return;
    }

    const TArray<FAnimNotifyEvent>& RecentNotifies = PreviewController->GetRecentFiredNotifyEvents();
    if (RecentNotifies.empty())
    {
        ImGui::TextDisabled("No notify fired during the latest preview update.");
        return;
    }

    constexpr int32 MaxVisibleEntries = 6;
    const int32 VisibleCount = std::min(static_cast<int32>(RecentNotifies.size()), MaxVisibleEntries);
    for (int32 Offset = 0; Offset < VisibleCount; ++Offset)
    {
        const int32 RecentIndex = static_cast<int32>(RecentNotifies.size()) - 1 - Offset;
        const FAnimNotifyEvent& NotifyEvent = RecentNotifies[RecentIndex];
        const FString NotifyName = NotifyEvent.Name.IsValid() ? NotifyEvent.Name.ToString() : FString("(Unnamed)");
        ImGui::BulletText("%s @ %.3fs", NotifyName.c_str(), NotifyEvent.Time);
    }
}
