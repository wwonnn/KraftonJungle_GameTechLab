#pragma once

#include "Animation/AnimData/AnimNotifyTypes.h"
#include "Object/Object.h"

class UAnimSequence;
class USkeletalMeshComponent;

class UAnimNotify : public UObject
{
public:
    DECLARE_CLASS(UAnimNotify, UObject)

    virtual FString GetNotifyName(const FAnimNotifyEvent& NotifyEvent) const;
    virtual void Notify(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent);
};

class UAnimNotifyState : public UAnimNotify
{
public:
    DECLARE_CLASS(UAnimNotifyState, UAnimNotify)

    void Notify(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent) override;
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent);
    virtual void NotifyTick(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent, float DeltaTime);
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComponent, UAnimSequence* Animation, const FAnimNotifyEvent& NotifyEvent);
};
