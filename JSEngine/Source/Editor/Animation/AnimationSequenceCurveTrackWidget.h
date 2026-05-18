#pragma once

class UAnimSequence;
class FAnimationSequenceEditorState;
struct FAnimationSequenceTimelineGeometry;

class FAnimationSequenceCurveTrackWidget
{
public:
    void RenderRows(
        const UAnimSequence* Sequence,
        FAnimationSequenceEditorState& State,
        const FAnimationSequenceTimelineGeometry& Geometry,
        float SectionTop) const;
};
