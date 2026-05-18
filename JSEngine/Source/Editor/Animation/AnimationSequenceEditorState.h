#pragma once

#include "Animation/AnimData/FrameRate.h"
#include "Core/CoreMinimal.h"

class UAnimSequence;

class FAnimationSequenceEditorState
{
public:
    void Reset();
    void InitializeFromSequence(const UAnimSequence* Sequence, float InSequenceLength, bool bInLoop, float InPlayRate);

    double GetDisplayFPS() const;
    bool HasTimelineData() const;
    float GetMaxTime() const;
    float GetMinimumVisibleRange() const;
    float GetVisibleRange() const;

    float ClampTime(float InTime) const;
    float SnapTimeToFrame(float InTime) const;
    float ClampOrSnapTime(float InTime) const;

    int32 TimeToFrameIndex(float InTime) const;
    float FrameToTime(int32 FrameIndex) const;

    void SetCurrentTime(float InTime, bool bApplySnap);
    void SetVisibleRange(float InStartTime, float InEndTime);
    void PanVisibleRange(float DeltaTime);
    void ZoomVisibleRange(float AnchorTime, float ZoomFactor);

    int32 ChooseMajorFrameStep(float TimelinePixelWidth) const;
    int32 ChooseMinorFrameStep(float TimelinePixelWidth) const;
    bool HasSelectedNotify() const
    {
        return SelectedNotifyTrackIndex >= 0 && SelectedNotifyEventIndex >= 0;
    }

public:
    float CurrentTime = 0.0f;
    float SequenceLength = 0.0f;
    FFrameRate DisplayFrameRate = FFrameRate();
    int32 TotalFrames = 0;
    bool bLoop = true;
    float PlayRate = 1.0f;
    bool bSnapToFrames = true;
    float VisibleTimeStart = 0.0f;
    float VisibleTimeEnd = 1.0f;
    float PreviewPaneHeight = 0.0f;
    float TrackOutlinerWidth = 0.0f;
    float SequencerScrollY = 0.0f;
    bool bNotifiesExpanded = true;
    bool bCurvesExpanded = true;
    bool bShowMorphTargetCurves = true;
    bool bShowMaterialCurves = true;
    bool bShowAttributeCurves = true;
    bool bShowUnknownCurves = false;
    int32 SelectedNotifyTrackIndex = -1;
    int32 SelectedNotifyEventIndex = -1;
    int32 HoveredNotifyTrackIndex = -1;
    int32 HoveredNotifyEventIndex = -1;
    int32 DraggedNotifyTrackIndex = -1;
    int32 DraggedNotifyEventIndex = -1;
    int32 SelectedCurveIndex = -1;
    int32 HoveredCurveIndex = -1;
    bool bDraggingNotify = false;
    float DraggedNotifyGrabOffsetTime = 0.0f;
};
