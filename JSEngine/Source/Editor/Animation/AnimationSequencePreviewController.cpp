#include "Editor/Animation/AnimationSequencePreviewController.h"

void FAnimationSequencePreviewController::SetSequence(const FString& InSequencePath, UAnimSequence* InSequence)
{
    SequencePath = InSequencePath;
    Sequence = InSequence;
}
