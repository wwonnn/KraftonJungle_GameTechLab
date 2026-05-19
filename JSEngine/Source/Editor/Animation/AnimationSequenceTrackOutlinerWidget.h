#pragma once

#include "Editor/Animation/AnimationSequenceSequencerVisibleLayout.h"

class FAnimationSequenceEditorState;
class FAnimationSequenceEditorDocument;

class FAnimationSequenceTrackOutlinerWidget
{
public:
    void Render(
        FAnimationSequenceEditorState& State,
        FAnimationSequenceEditorDocument* Document,
        const FAnimationSequenceSequencerVisibleLayout& VisibleLayout);
};
