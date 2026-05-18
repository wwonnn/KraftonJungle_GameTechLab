#pragma once

class FAnimationSequenceEditorState;
class FAnimationSequencePreviewController;

class FAnimationSequenceTimelineWidget
{
public:
    void Render(FAnimationSequenceEditorState& State, FAnimationSequencePreviewController* PreviewController);
};
