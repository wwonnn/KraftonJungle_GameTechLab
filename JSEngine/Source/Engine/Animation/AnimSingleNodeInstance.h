#pragma once

#include "Animation/AnimInstance.h"
#include "Generated/AnimSingleNodeInstance.generated.h"

// Editor에서 단일 애니메이션을 재생하기 위한 AnimInstance (ex. Viewer)
UCLASS()
class UAnimSingleNodeInstance : public UAnimInstance
{
public:
    GENERATED_BODY()

public:
    void UpdateAnimation(float DeltaTime) override;
    void EvaluatePose(FSkeletonPose& OutPose) override;
};
