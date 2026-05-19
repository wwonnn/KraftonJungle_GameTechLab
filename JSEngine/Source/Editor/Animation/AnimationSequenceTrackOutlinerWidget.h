#pragma once

#include "Editor/Animation/AnimationSequenceCurveFilter.h"

class FAnimationSequenceEditorState;
class FAnimationSequenceEditorDocument;

class FAnimationSequenceTrackOutlinerWidget
{
public:
    void Render(
        FAnimationSequenceEditorState& State,
        FAnimationSequenceEditorDocument* Document,
        const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups,
        const TArray<FAnimationSequenceCurveViewGroup>& AttributeCurveGroups);
};
