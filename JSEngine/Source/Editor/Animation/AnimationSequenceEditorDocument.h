#pragma once

#include "Animation/AnimData/AnimNotifyTypes.h"
#include "Animation/AnimNotifyPayloadParser.h"
#include "Editor/Animation/AnimationSequenceNotifyValidation.h"
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

    bool CanSave() const override;
    bool Save() override;
    bool CanClose() const override { return true; }
    bool IsDetachedSupported() const override { return false; }

    const FString& GetSequencePath() const { return SequencePath; }
    UAnimSequence* GetSequence() const { return Sequence; }
    FAnimationSequenceEditorState& GetEditorState() { return EditorState; }
    const FAnimationSequenceEditorState& GetEditorState() const { return EditorState; }

    int32 GetNotifyTrackCount() const;
    const FAnimNotifyTrack* GetNotifyTrack(int32 TrackIndex) const;
    FAnimNotifyTrack* GetNotifyTrack(int32 TrackIndex);
    int32 AddNotifyTrack();
    bool RenameNotifyTrack(int32 TrackIndex, const FName& NewName);
    bool DeleteNotifyTrack(int32 TrackIndex);
    bool MoveNotifyTrack(int32 FromIndex, int32 ToIndex);
    const FAnimNotifyEvent* GetSelectedNotify() const;
    FAnimNotifyEvent* GetSelectedNotify();
    bool AddNotifyAtTime(int32 TrackIndex, float TimeSeconds);
    bool DuplicateSelectedNotify();
    bool CopySelectedNotify();
    bool CanPasteNotify() const;
    bool PasteNotifyToTrackAtTime(int32 TrackIndex, float TimeSeconds);
    bool MoveSelectedNotifyToTrack(int32 TargetTrackIndex);
    bool DeleteSelectedNotify();
    bool SetSelectedNotifyTime(float TimeSeconds, bool bApplySnap);
    bool SetSelectedNotifyDuration(float DurationSeconds);
    bool SetSelectedNotifyName(const FName& Name);
    bool SetSelectedNotifyColor(const FColor& Color);
    bool SetSelectedNotifyType(EAnimNotifyEventType EventType);
    bool SetSelectedNotifyClassName(const FString& NotifyClassName);
    bool SetSelectedNotifyPayload(const FString& Payload);
    FAnimNotifyPayloadParser GetSelectedNotifyPayloadParser() const;
    bool SetSelectedNotifyPayloadStringValue(const FString& Key, const FString& Value);
    bool SetSelectedNotifyPayloadNameValue(const FString& Key, const FName& Value);
    bool SetSelectedNotifyPayloadFloatValue(const FString& Key, float Value);
    bool SetSelectedNotifyPayloadBoolValue(const FString& Key, bool Value);
    bool ClearSelectedNotifyPayloadValue(const FString& Key);
    void SelectNotify(int32 TrackIndex, int32 EventIndex);
    void ClearNotifySelection();
    void MarkDirty();
    bool IsDirty() const { return bDirty; }
    FAnimNotifyValidationReport BuildSelectedNotifyValidationReport() const;
    FAnimNotifyValidationReport BuildDocumentNotifyValidationReport() const;
    const FString& GetLastNotifyValidationStatusText() const { return LastNotifyValidationStatusText; }

private:
    struct FNotifyClipboard
    {
        bool bValid = false;
        EAnimNotifyEventType EventType = EAnimNotifyEventType::Notify;
        FName Name;
        float Time = 0.0f;
        float Duration = 0.0f;
        FColor Color;
        FString NotifyClassName;
        FString Payload;
        int32 SourceTrackIndex = -1;
    };

    void SyncEditorState();
    void ValidateNotifySelection();

private:
    UEditorEngine* EditorEngine = nullptr;
    FString SequencePath;
    UAnimSequence* Sequence = nullptr;
    FEditorTabId TabId;
    FString TabLabel;
    FAnimationSequenceEditorState EditorState;
    std::unique_ptr<FAnimationSequenceEditorWidget> Widget;
    std::unique_ptr<FAnimationSequencePreviewController> PreviewController;
    FNotifyClipboard NotifyClipboard;
    bool bDirty = false;
    FString LastNotifyValidationStatusText;
};
