#pragma once

#include "Animation/AnimNotify.h"
#include "Audio/AudioSystem.h"

class AActor;
class UPrimitiveComponent;

class UAnimNotify_PlaySFX : public UAnimNotify
{
public:
    DECLARE_CLASS(UAnimNotify_PlaySFX, UAnimNotify)

    void Notify(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent) override;
};

class UAnimNotifyState_PlayLoopingSFX : public UAnimNotifyState
{
public:
    DECLARE_CLASS(UAnimNotifyState_PlayLoopingSFX, UAnimNotifyState)

    ~UAnimNotifyState_PlayLoopingSFX() override;

    void NotifyBegin(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent) override;
    void NotifyTick(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent, float DeltaTime) override;
    void NotifyEnd(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent) override;

private:
    void Cleanup();

private:
    FAudioHandle ActiveHandle = 0;
    bool bSpatialized = false;
};

class UAnimNotifyState_AttackWindow : public UAnimNotifyState
{
public:
    DECLARE_CLASS(UAnimNotifyState_AttackWindow, UAnimNotifyState)

    ~UAnimNotifyState_AttackWindow() override;

    void NotifyBegin(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent) override;
    void NotifyEnd(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent) override;

private:
    void Cleanup();

private:
    AActor* CachedOwnerActor = nullptr;
    UPrimitiveComponent* CachedPrimitiveComponent = nullptr;
    FString ActiveAttackTag;
    FString ActiveAttackTagStateKey;
    bool bWindowAcquired = false;
    bool bAttackTagAcquired = false;
};
