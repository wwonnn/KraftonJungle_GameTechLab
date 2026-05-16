#pragma once

#include "Editor/UI/EditorWidget.h"

class UAnimSequence;
class FAnimationSequencePreviewController;

class FAnimationSequenceEditorWidget : public FEditorWidget
{
public:
    void Render(float DeltaTime) override;

    void BindDocumentContext(
        const FString& InSequencePath,
        UAnimSequence* InSequence,
        FAnimationSequencePreviewController* InPreviewController);

private:
    FString SequencePath;
    UAnimSequence* Sequence = nullptr;
    FAnimationSequencePreviewController* PreviewController = nullptr;
};
