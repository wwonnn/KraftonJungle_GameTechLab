#pragma once

#include "Editor/Animation/AnimationSequenceNotifyLaneWidget.h"
#include "Editor/Animation/AnimationSequenceTimelineWidget.h"
#include "Editor/UI/EditorWidget.h"

class UAnimSequence;
class FAnimationSequenceEditorState;
class FAnimationSequencePreviewController;

class FAnimationSequenceEditorWidget : public FEditorWidget
{
public:
    void Render(float DeltaTime) override;

    void BindDocumentContext(
        const FString& InSequencePath,
        UAnimSequence* InSequence,
        FAnimationSequencePreviewController* InPreviewController,
        FAnimationSequenceEditorState* InEditorState);

private:
    FString SequencePath;
    UAnimSequence* Sequence = nullptr;
    FAnimationSequencePreviewController* PreviewController = nullptr;
    FAnimationSequenceEditorState* EditorState = nullptr;
    FAnimationSequenceTimelineWidget TimelineWidget;
    FAnimationSequenceNotifyLaneWidget NotifyLaneWidget;
};
