#pragma once

#include "Editor/Animation/AnimationSequenceCurveFilter.h"

class FAnimationSequenceEditorState;
struct FAnimationSequenceTimelineGeometry;

class FAnimationSequenceCurveTrackWidget
{
public:
    void RenderRows(
        FAnimationSequenceEditorState& State,
        const FAnimationSequenceTimelineGeometry& Geometry,
        float SectionTop,
        const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups) const;
};
