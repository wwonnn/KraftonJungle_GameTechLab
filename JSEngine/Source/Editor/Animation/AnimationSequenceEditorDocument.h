#pragma once

#include "Editor/Animation/AnimationSequenceEditorState.h"
#include "Editor/Animation/AnimationSequenceEditorWidget.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"
#include "Editor/Document/EditorDocument.h"

#include <memory>

class UEditorEngine;
class UAnimSequence;

class FAnimationSequenceEditorDocument : public FEditorDocument
{
public:
    FAnimationSequenceEditorDocument() = default;
    ~FAnimationSequenceEditorDocument() override;

    bool Initialize(UEditorEngine* InEditorEngine, const FString& InSequencePath, UAnimSequence* InSequence);

    const FEditorTabId& GetTabId() const override { return TabId; }
    const FString& GetTabLabel() const override { return TabLabel; }

    void Tick(float DeltaTime) override;
    void RenderEmbedded(float DeltaTime) override;
    void RenderToolbar() override;
    void BuildCommandList(FEditorCommandList& OutCommands) override;
    FSceneViewport* GetSceneViewport() override;
    const FSceneViewport* GetSceneViewport() const override;

    bool CanSave() const override { return false; }
    bool Save() override { return false; }
    bool CanClose() const override { return true; }
    bool IsDetachedSupported() const override { return false; }

    const FString& GetSequencePath() const { return SequencePath; }
    UAnimSequence* GetSequence() const { return Sequence; }

private:
    void SyncEditorState();

private:
    UEditorEngine* EditorEngine = nullptr;
    FString SequencePath;
    UAnimSequence* Sequence = nullptr;
    FEditorTabId TabId;
    FString TabLabel;
    FAnimationSequenceEditorState EditorState;
    std::unique_ptr<FAnimationSequenceEditorWidget> Widget;
    std::unique_ptr<FAnimationSequencePreviewController> PreviewController;
};
