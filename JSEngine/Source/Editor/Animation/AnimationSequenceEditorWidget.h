#pragma once

#include "Editor/Animation/AnimationSequenceNotifyValidation.h"
#include "Editor/Animation/AnimationSequenceCurveTrackWidget.h"
#include "Editor/Animation/AnimationSequenceCurveFilter.h"
#include "Editor/Animation/AnimationSequenceNotifyLaneWidget.h"
#include "Editor/Animation/AnimationSequenceTrackOutlinerWidget.h"
#include "Editor/Animation/AnimationSequenceTimelineWidget.h"
#include "Editor/UI/EditorWidget.h"

#include <array>

class UAnimSequence;
class UTexture;
class FAnimationSequenceEditorState;
class FAnimationSequencePreviewController;
class FAnimationSequenceEditorDocument;
class FAnimNotifyPayloadParser;
enum class EAnimNotifySemanticFieldId : uint8;
struct FAnimNotifyEvent;
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
    void RenderPreviewToolbarOverlay(const ImVec2& Min, const ImVec2& Max) const;
    void RenderSequencerRegion(float SequencerHeight, float MaxPreviewHeight);
    void RenderSequencerSplitPane(
        float SplitPaneHeight,
        float DetailsPaneHeight,
        float MaxOutlinerWidth,
        const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups);
    void RenderTrackOutlinerPane(
        float Width,
        float Height,
        const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups);
    void RenderTimelineCanvasPane(
        float Width,
        float Height,
        const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups);
    void RenderTimelineFooter(float Width);
    void RenderSelectionDetailsPane(float Height);
    void EnsurePlaybackControlIconsLoaded();
    bool DrawPlaybackControlButton(
        const char* ButtonId,
        UTexture* IconTexture,
        const char* FallbackLabel,
        const char* Tooltip,
        const ImVec2& Size,
        bool bSelected = false) const;
    void RenderTransportControls(bool bCanTimelineControl, bool bCanPlaybackControl);
    void RenderNotifyDetailsPanel();
    void RenderCurveInspectionPanel() const;
    void RenderRecentNotifySummary() const;
    void RenderFooterStatus() const;
    void RenderNotifyValidationSummary(const FAnimNotifyValidationReport& Report) const;
    void RenderNotifyValidationIssues(
        const FAnimNotifyValidationReport& Report,
        EAnimNotifyValidationField Field) const;
    void RenderStructuredNotifyPayloadEditor(
        const FString& NotifyClassName,
        const FAnimNotifyValidationReport& Report);
    void RenderRawNotifyPayloadEditor(
        const FString& NotifyClassName,
        const FAnimNotifyValidationReport& Report,
        bool bRenderValidation);
    void SyncNotifyDetailsBuffers(const FAnimNotifyEvent& NotifyEvent);
    void ResetNotifyPayloadFieldBuffers();
    void RefreshNotifyPayloadFieldBuffers(const FAnimNotifyPayloadParser& Payload);
    std::array<char, 256>* GetNotifyTextFieldBuffer(EAnimNotifySemanticFieldId SemanticId);

private:
    FAnimationSequenceEditorDocument* Document = nullptr;
    FString SequencePath;
    UAnimSequence* Sequence = nullptr;
    FAnimationSequencePreviewController* PreviewController = nullptr;
    FAnimationSequenceEditorState* EditorState = nullptr;
    FAnimationSequenceTimelineWidget TimelineWidget;
    FAnimationSequenceNotifyLaneWidget NotifyLaneWidget;
    FAnimationSequenceTrackOutlinerWidget TrackOutlinerWidget;
    FAnimationSequenceCurveTrackWidget CurveTrackWidget;
    struct FPlaybackControlIconCache
    {
        bool bAttemptedLoad = false;
        UTexture* JumpToStart = nullptr;
        UTexture* StepPrevious = nullptr;
        UTexture* PlayForward = nullptr;
        UTexture* Pause = nullptr;
        UTexture* Stop = nullptr;
        UTexture* StepNext = nullptr;
        UTexture* JumpToEnd = nullptr;
        UTexture* LoopEnabled = nullptr;
        UTexture* LoopDisabled = nullptr;
    } PlaybackControlIcons;
    FString NotifyDetailsBoundStableId;
    std::array<char, 256> NotifyNameEditBuffer = {};
    std::array<char, 256> NotifySoundCueEditBuffer = {};
    std::array<char, 256> NotifySocketNameEditBuffer = {};
    std::array<char, 256> NotifyComponentNameEditBuffer = {};
    std::array<char, 256> NotifyAttackIdEditBuffer = {};
    std::array<char, 512> NotifyPayloadEditBuffer = {};
};
