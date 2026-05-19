#include "AnimSingleNodeInstance.h"

#include "Object/ObjectFactory.h"

REGISTER_FACTORY(UAnimSingleNodeInstance)

void UAnimSingleNodeInstance::UpdateAnimation(float DeltaTime)
{
    UAnimInstance::UpdateAnimation(DeltaTime);
}

void UAnimSingleNodeInstance::EvaluatePose(FSkeletonPose& OutPose)
{
    if (!CurrentSequence)
    {
        InitializeReferencePose(OutPose);
        return;
    }

    EvaluatePoseAtTime(CurrentSequence, CurrentTime, OutPose.LocalTransforms);
}
