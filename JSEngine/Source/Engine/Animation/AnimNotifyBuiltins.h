#pragma once

#include "Animation/AnimNotify.h"
#include "Audio/AudioSystem.h"
#include "Generated/AnimNotifyBuiltins.generated.h"

class AActor;
class UPrimitiveComponent;

UCLASS()
class UAnimNotify_PlaySFX : public UAnimNotify
{
public:
    GENERATED_BODY()

    void Notify(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent) override;
};

UCLASS()
class UAnimNotifyState_PlayLoopingSFX : public UAnimNotifyState
{
public:
    GENERATED_BODY()

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

UCLASS()
class UAnimNotifyState_AttackWindow : public UAnimNotifyState
{
public:
    GENERATED_BODY()

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
