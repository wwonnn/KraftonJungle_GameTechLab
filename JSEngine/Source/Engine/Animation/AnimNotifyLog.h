#pragma once

#include "Animation/AnimNotify.h"

class UAnimNotifyLog : public UAnimNotify
{
public:
    DECLARE_CLASS(UAnimNotifyLog, UAnimNotify)

    void Notify(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent) override;
};

class UAnimNotifyStateLog : public UAnimNotifyState
{
public:
    DECLARE_CLASS(UAnimNotifyStateLog, UAnimNotifyState)

    void NotifyBegin(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent) override;
    void NotifyTick(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent, float DeltaTime) override;
    void NotifyEnd(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent) override;
};
