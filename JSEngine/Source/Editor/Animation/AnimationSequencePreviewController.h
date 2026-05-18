#pragma once

#include "Animation/AnimData/FrameRate.h"
#include "Core/CoreMinimal.h"
#include "Editor/Viewport/FSceneViewport.h"
#include "Editor/Viewport/SkeletalMeshViewportClient.h"

class UEditorEngine;
class UAnimSequence;
class USkeletalMesh;
class USkeletalMeshComponent;
class ASkeletalMeshActor;
class UWorld;
struct ID3D11ShaderResourceView;

class FAnimationSequencePreviewController
{
public:
    FAnimationSequencePreviewController() = default;
    ~FAnimationSequencePreviewController();

    bool Initialize(UEditorEngine* InEditorEngine, const FString& InSequencePath, UAnimSequence* InSequence);
    void Shutdown();
    void Tick(float DeltaTime);

    void SetCurrentTime(float InTime);
    float GetCurrentTime() const { return CurrentTime; }
    float GetLength() const;
    FFrameRate GetFrameRate() const;
    int32 GetFrameCount() const;
    int32 GetCurrentFrameIndex() const;

    void Play();
    void Pause();
    void Stop();
    void StepToNextFrame();
    void StepToPreviousFrame();
    void JumpToStart();
    void JumpToEnd();

    void SetLooping(bool bInLooping);
    bool IsLooping() const { return bLooping; }

    void SetPlayRate(float InPlayRate);
    float GetPlayRate() const { return PlayRate; }

    bool IsPlaying() const { return bPlaying; }
    bool HasSequence() const { return Sequence != nullptr; }
    bool HasValidPreview() const;

    const FString& GetPreviewStatusText() const { return PreviewStatusText; }
    const FString& GetTimelineStatusText() const { return TimelineStatusText; }

    void SetViewportSize(int32 InWidth, int32 InHeight);
    ID3D11ShaderResourceView* GetPreviewSRV() const;

    FSceneViewport* GetSceneViewport() { return HasValidPreview() ? &PreviewViewport : nullptr; }
    const FSceneViewport* GetSceneViewport() const { return HasValidPreview() ? &PreviewViewport : nullptr; }

private:
    bool ResolvePreviewMeshPath(FString& OutMeshPath) const;
    bool InitializePreviewWorld();
    bool InitializePreviewActor();
    void InitializeViewportClient();
    void ConfigurePreviewCamera();
    void RefreshPreviewPose(float DeltaTime);

private:
    static constexpr uint32 InvalidPreviewResourceIndex = ~0u;
    inline static uint32 NextPreviewResourceIndex = 0;

    UEditorEngine* EditorEngine = nullptr;
    FString SequencePath;
    UAnimSequence* Sequence = nullptr;
    FString PreviewMeshPath;
    USkeletalMesh* PreviewMesh = nullptr;

    FName PreviewWorldHandle;
    UWorld* PreviewWorld = nullptr;
    ASkeletalMeshActor* PreviewActor = nullptr;
    USkeletalMeshComponent* PreviewComponent = nullptr;

    FSceneViewport PreviewViewport;
    FSkeletalMeshViewportClient PreviewViewportClient;
    uint32 PreviewResourceIndex = InvalidPreviewResourceIndex;
    int32 ViewportWidth = 512;
    int32 ViewportHeight = 512;

    float CurrentTime = 0.0f;
    float PlayRate = 1.0f;
    bool bPlaying = false;
    bool bLooping = true;
    bool bInitialized = false;
    bool bPoseDirty = false;

    FString PreviewStatusText = "Preview is not initialized.";
    FString TimelineStatusText = "Timeline controls are available after preview initialization.";
};
