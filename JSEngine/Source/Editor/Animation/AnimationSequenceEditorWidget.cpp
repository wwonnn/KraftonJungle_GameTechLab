#include "Editor/Animation/AnimationSequenceEditorWidget.h"

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Editor/Animation/AnimationSequenceEditorState.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"
#include "Editor/Animation/AnimationSequenceViewerUtils.h"
#include "Editor/EditorEngine.h"
#include "Editor/Viewport/EditorViewportClient.h"
#include "Editor/Viewport/FSceneViewport.h"
#include "Engine/Runtime/WindowsWindow.h"
#include "Render/Common/RenderTypes.h"

#include "ImGui/imgui.h"

#include <algorithm>

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

namespace
{
    constexpr float PreviewSectionMinHeight = 180.0f;
    constexpr float TransportSectionMinHeight = 180.0f;
    constexpr float SectionSplitterHeight = 6.0f;
    constexpr float PreviewOverlayMargin = 10.0f;
    constexpr float PreviewOverlayPadding = 8.0f;
    constexpr float PreviewOverlayLineSpacing = 2.0f;

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
        const ImVec2 OverlayMin(Min.x + PreviewOverlayMargin, Min.y + PreviewOverlayMargin);
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
}

void FAnimationSequenceEditorWidget::BindDocumentContext(
    const FString& InSequencePath,
    UAnimSequence* InSequence,
    FAnimationSequencePreviewController* InPreviewController,
    FAnimationSequenceEditorState* InEditorState)
{
    SequencePath = InSequencePath;
    Sequence = InSequence;
    PreviewController = InPreviewController;
    EditorState = InEditorState;
}

void FAnimationSequenceEditorWidget::Render(float DeltaTime)
{
    (void)DeltaTime;

    const TArray<FString> PreviewOverlayLines = BuildPreviewOverlayLines();

    ImGui::Spacing();
    ImGui::TextDisabled("Preview");

    const float StatusReserveHeight = ImGui::GetTextLineHeightWithSpacing() * 2.0f + 18.0f;
    const float MinSplitHeight = PreviewSectionMinHeight + TransportSectionMinHeight + SectionSplitterHeight;
    const float SplitLayoutHeight = EditorState
        ? std::max(ImGui::GetContentRegionAvail().y - StatusReserveHeight, MinSplitHeight)
        : std::max(240.0f, ImGui::GetContentRegionAvail().y * 0.45f);
    const float MaxPreviewHeight = EditorState
        ? std::max(PreviewSectionMinHeight, SplitLayoutHeight - TransportSectionMinHeight - SectionSplitterHeight)
        : SplitLayoutHeight;

    if (EditorState && EditorState->PreviewPaneHeight <= 0.0f)
    {
        EditorState->PreviewPaneHeight = SplitLayoutHeight * 0.55f;
    }

    const float PreviewHeight = EditorState
        ? std::clamp(EditorState->PreviewPaneHeight, PreviewSectionMinHeight, MaxPreviewHeight)
        : SplitLayoutHeight;
    const float TransportPanelHeight = EditorState
        ? std::max(TransportSectionMinHeight, SplitLayoutHeight - PreviewHeight - SectionSplitterHeight)
        : 0.0f;

    if (EditorState)
    {
        EditorState->PreviewPaneHeight = PreviewHeight;
    }

    RenderPreviewPane(PreviewHeight, PreviewOverlayLines);

    if (EditorState)
    {
        RenderTransportAndTimelinePanel(TransportPanelHeight, MaxPreviewHeight);
    }

    ImGui::Spacing();
    RenderFooterStatus();
}

TArray<FString> FAnimationSequenceEditorWidget::BuildPreviewOverlayLines() const
{
    TArray<FString> PreviewOverlayLines;
    PreviewOverlayLines.push_back("Animation Sequence");
    PreviewOverlayLines.push_back("Asset Path: " + (SequencePath.empty() ? FString("(none)") : SequencePath));
    PreviewOverlayLines.push_back(
        "Sequence Name: " +
        (AnimationSequenceViewer::IsLiveObject(Sequence) ? Sequence->GetName() : FString("(unloaded)")));

    const UAnimDataModel* DataModel = AnimationSequenceViewer::GetValidAnimDataModel(Sequence);
    if (DataModel)
    {
        PreviewOverlayLines.push_back(
            "Frame Rate: " +
            std::to_string(DataModel->FrameRate.Numerator) +
            " / " +
            std::to_string(DataModel->FrameRate.Denominator));
        PreviewOverlayLines.push_back("Number Of Frames: " + std::to_string(DataModel->NumberOfFrames));
        PreviewOverlayLines.push_back("Number Of Keys: " + std::to_string(DataModel->NumberOfKeys));
        PreviewOverlayLines.push_back(
            "Bone Track Count: " + std::to_string(static_cast<int32>(DataModel->BoneAnimationTracks.size())));
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

void FAnimationSequenceEditorWidget::RenderTransportAndTimelinePanel(
    float TransportPanelHeight,
    float MaxPreviewHeight)
{
    ImDrawList* SplitterDrawList = ImGui::GetWindowDrawList();
    const float SplitterWidth = std::max(ImGui::GetContentRegionAvail().x, 1.0f);
    ImGui::InvisibleButton("##AnimSequencePreviewTransportSplitter", ImVec2(SplitterWidth, SectionSplitterHeight));
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
            PreviewSectionMinHeight,
            MaxPreviewHeight);
    }

    ImGui::BeginChild("##AnimationSequenceTransportPanel", ImVec2(0.0f, TransportPanelHeight), true);
    ImGui::TextDisabled("Transport");

    const bool bCanTimelineControl =
        PreviewController != nullptr && EditorState != nullptr && EditorState->HasTimelineData();
    const bool bCanPlaybackControl = PreviewController != nullptr && PreviewController->HasValidPreview();

    RenderTransportControls(bCanTimelineControl, bCanPlaybackControl);
    RenderPlaybackSummary();

    ImGui::Spacing();
    ImGui::TextDisabled("Timeline");
    TimelineWidget.Render(*EditorState, PreviewController);

    ImGui::Spacing();
    NotifyLaneWidget.Render(*EditorState);
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
