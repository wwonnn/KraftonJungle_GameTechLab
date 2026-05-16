#include "AnimSingleNodeInstance.h"

#include "Object/ObjectFactory.h"

DEFINE_CLASS(UAnimSingleNodeInstance, UAnimInstance)
REGISTER_FACTORY(UAnimSingleNodeInstance)

void UAnimSingleNodeInstance::UpdateAnimation(float DeltaTime)
{
    if (!bPlaying || CurrentSequence == nullptr || !CurrentSequence->DataModel)
    {
        return;
    }

    const UAnimDataModel* Model = CurrentSequence->DataModel;
    const float Length = (float)(Model->NumberOfKeys - 1) / Model->FrameRate.AsDecimal();

    CurrentTime += DeltaTime * PlayRate;

    if (bLoop && Length > 0.0f)
    {
        CurrentTime = std::fmod(CurrentTime, Length);

        // PlayRate < 0
        if (CurrentTime < 0.0f)
            CurrentTime += Length;
    }
    else
    {
        CurrentTime = std::clamp(CurrentTime, 0.0f, Length);

        if (CurrentTime >= Length)
            bPlaying = false;
    }
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
