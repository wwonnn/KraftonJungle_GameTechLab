#pragma once

class UAnimSequence;
class FAnimationSequenceEditorState;
class FAnimationSequenceEditorDocument;

class FAnimationSequenceTrackOutlinerWidget
{
public:
    void Render(FAnimationSequenceEditorState& State, FAnimationSequenceEditorDocument* Document, const UAnimSequence* Sequence);
};
