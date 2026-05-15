#pragma once

#include "Animation/AnimInstance.h"

// Editor에서 단일 애니메이션을 재생하기 위한 AnimInstance (ex. Viewer)
class UAnimSingleNodeInstance : public UAnimInstance
{
public:
    DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)

public:
    void UpdateAnimation(float DeltaTime) override;
    void EvaluatePose(FSkeletonPose& OutPose) override;
};
