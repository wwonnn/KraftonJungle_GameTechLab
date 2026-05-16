#pragma once

#include "Core/CoreMinimal.h"

class UAnimSequence;

class FAnimationSequencePreviewController
{
public:
    void SetSequence(const FString& InSequencePath, UAnimSequence* InSequence);

    bool HasSequence() const { return Sequence != nullptr; }
    const FString& GetPreviewStatusText() const { return PreviewStatusText; }
    const FString& GetTimelineStatusText() const { return TimelineStatusText; }

private:
    FString SequencePath;
    UAnimSequence* Sequence = nullptr;
    FString PreviewStatusText = "Preview mesh binding is pending importer metadata integration.";
    FString TimelineStatusText = "Timeline and playback controls are deferred to Priority 1.";
};
