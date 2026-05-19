#pragma once

class FAnimationSequenceEditorState;
class FAnimationSequenceEditorDocument;
struct FAnimationSequenceTimelineGeometry;

class FAnimationSequenceNotifyLaneWidget
{
public:
    void RenderRows(
        FAnimationSequenceEditorState& State,
        FAnimationSequenceEditorDocument* Document,
        const FAnimationSequenceTimelineGeometry& Geometry,
        float SectionTop);
};
