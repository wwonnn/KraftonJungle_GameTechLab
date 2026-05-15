#include "AnimInstance.h"

void UAnimInstance::UpdateAnimation(float DeltaTime)
{
    if (!bPlaying || CurrSequence == nullptr)
    {
        return;
    }

    CurrentTime += DeltaTime * PlayRate;

    const float Length = CurrSequence->DataModel->SequenceLength;

    if (bLoop)
    {
        while (CurrentTime > Length)
        {
            CurrentTime -= Length;
        }

        while (CurrentTime < 0.0f)
        {
            CurrentTime += Length;
        }
    }
    else
    {
        CurrentTime = std::clamp(CurrentTime, 0.0f, Length);
    }
}

void UAnimInstance::EvaluatePose(FSkeletonPose& OutPose)
{
}

void UAnimInstance::InitializeReferencePose()
{
}

void UAnimInstance::EvaluatePoseAtTime(float DeltaTime, TArray<FTransform>& OutLocalTransforms)
{
}

void UAnimInstance::BlendPoses(const FSkeletonPose& PoseA, const FSkeletonPose& PoseB, float BlendFactor, FSkeletonPose& OutPose)
{
}
