#pragma once

#include "Editor/Animation/AnimationSequenceNotifyLaneWidget.h"
#include "Editor/Animation/AnimationSequenceTimelineWidget.h"
#include "Editor/UI/EditorWidget.h"

class UAnimSequence;
class FAnimationSequenceEditorState;
class FAnimationSequencePreviewController;
class FAnimationSequenceEditorDocument;
struct ImVec2;

class FAnimationSequenceEditorWidget : public FEditorWidget
{
public:
    void Render(float DeltaTime) override;

    void BindDocumentContext(
        FAnimationSequenceEditorDocument* InDocument,
        const FString& InSequencePath,
        UAnimSequence* InSequence,
        FAnimationSequencePreviewController* InPreviewController,
        FAnimationSequenceEditorState* InEditorState);

private:
    TArray<FString> BuildPreviewOverlayLines() const;
    void RenderPreviewPane(float PreviewHeight, const TArray<FString>& PreviewOverlayLines);
    void RenderPreviewImage(const ImVec2& PreviewSize, const TArray<FString>& PreviewOverlayLines);
    void RenderPreviewFallback(const ImVec2& PreviewSize, const TArray<FString>& PreviewOverlayLines) const;
    void SyncEmbeddedViewportRectAndFocus(const ImVec2& Min, const ImVec2& Max, bool bViewportClicked);
    void RenderTransportAndTimelinePanel(float TransportPanelHeight, float MaxPreviewHeight);
    void RenderTransportControls(bool bCanTimelineControl, bool bCanPlaybackControl);
    void RenderPlaybackSummary() const;
    void RenderNotifyDetailsPanel();
    void RenderCurveInspectionPanel() const;
    void RenderRecentNotifySummary() const;
    void RenderFooterStatus() const;

private:
    FAnimationSequenceEditorDocument* Document = nullptr;
    FString SequencePath;
    UAnimSequence* Sequence = nullptr;
    FAnimationSequencePreviewController* PreviewController = nullptr;
    FAnimationSequenceEditorState* EditorState = nullptr;
    FAnimationSequenceTimelineWidget TimelineWidget;
    FAnimationSequenceNotifyLaneWidget NotifyLaneWidget;
};
