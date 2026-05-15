#include "AnimInstance.h"

void UAnimInstance::UpdateAnimation(float DeltaTime)
{
    if (!bPlaying || CurrentSequence == nullptr || !CurrentSequence->DataModel)
    {
        return;
    }

	// Current Sequence Time
    const float Length = CurrentSequence->DataModel->SequenceLength;

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

	// Next Sequence Time (Blending)
    if (NextSequence && NextSequence->DataModel)
    {
        const float NextLength = NextSequence->DataModel->SequenceLength;

        NextTime += DeltaTime * PlayRate;

        if (NextLength > 0.0f)
        {
            NextTime = std::fmod(NextTime, NextLength);

			// PlayRate < 0
            if (NextTime < 0.0f)
                NextTime += NextLength;
        }
		
        BlendFactor += DeltaTime * BlendSpeed;
        BlendFactor = std::clamp(BlendFactor, 0.0f, 1.0f);

        if (BlendFactor >= 1.0f)
        {
            CurrentSequence = NextSequence;
            CurrentTime = NextTime;
            NextSequence = nullptr;
            NextTime = 0.0f;
            BlendFactor = 0.0f;
        }
    }
}

void UAnimInstance::EvaluatePose(FSkeletonPose& OutPose)
{
    if (!CurrentSequence)
    {
        InitializeReferencePose(OutPose);
        return;
    }

    if (NextSequence && BlendFactor > 0.0f)
    {
        // 두 포즈를 각각 계산한 후 블렌딩
        FSkeletonPose PoseA, PoseB;

        EvaluatePoseAtTime(CurrentSequence, CurrentTime, PoseA.LocalTransforms);
        EvaluatePoseAtTime(NextSequence, NextTime, PoseB.LocalTransforms);

        BlendPoses(PoseA, PoseB, BlendFactor, OutPose);
    }
    else
    {
        EvaluatePoseAtTime(CurrentSequence, CurrentTime, OutPose.LocalTransforms);
    }
}

void UAnimInstance::SetNextSequence(UAnimSequence* InNext, float InBlendSpeed)
{
    NextSequence = InNext;
    NextTime = 0.0f;
    BlendFactor = 0.0f;
    BlendSpeed = InBlendSpeed;
}

void UAnimInstance::InitializeReferencePose(FSkeletonPose& OutPose)
{
    if (!Owner)
        return;

	const USkeletalMesh* Mesh = Owner->GetSkeletalMesh();

	if (!Mesh || !Mesh->HasValidMeshData())
		return;

	const TArray<FBoneInfo>& Bones = Mesh->GetBones();
    const int32 BoneCount = static_cast<int32>(Bones.size());

	OutPose.LocalTransforms.resize(BoneCount);

	for (int32 i = 0; i < BoneCount; i++)
	{
        OutPose.LocalTransforms[i] = FTransform(Bones[i].LocalBindTransform);
    }
}

void UAnimInstance::EvaluatePoseAtTime(const UAnimSequence* Sequence, float CurrentTime, TArray<FTransform>& OutLocalTransforms)
{
    const UAnimDataModel* Model = Sequence->DataModel;
    const int32 BoneCount = Model->BoneTracks.size();

    OutLocalTransforms.resize(BoneCount);

	for (int32 i = 0; i < BoneCount; ++i)
    {
        const FBoneAnimationTrack& Track = Model->BoneTracks[i];
        int32 BoneIndex = Track.BoneIndex;

        FVector Position = InterpolateKeys(Track.PosKeys, CurrentTime);
        FQuat Rotation = InterpolateKeys(Track.RotKeys, CurrentTime);
        FVector Scale = InterpolateKeys(Track.ScaleKeys, CurrentTime);

        Rotation.Normalize(); // 보간 후 정규화 필수

        OutLocalTransforms[BoneIndex] = FTransform(Rotation, Position, Scale);
    }
}

FVector UAnimInstance::InterpolateKeys(const TArray<FAnimKeyVector>& Keys, float Time)
{
    if (Keys.empty())
        return FVector::ZeroVector;
    if (Keys.size() == 1)
        return Keys[0].Value;

    // Time이 범위 밖이면 클램프
    if (Time <= Keys[0].Time)
        return Keys[0].Value;
    if (Time >= Keys.back().Time)
        return Keys.back().Value;

    // 이진 탐색으로 구간 [Lo, Hi] 찾기
    int32 Lo = 0, Hi = Keys.size() - 1;
    while (Hi - Lo > 1)
    {
        int32 Mid = (Lo + Hi) / 2;
        if (Keys[Mid].Time <= Time)
            Lo = Mid;
        else
            Hi = Mid;
    }

    float Diff = Keys[Hi].Time - Keys[Lo].Time;
    if (Diff < 1e-4f)
        return Keys[Lo].Value;

    float Alpha = (Time - Keys[Lo].Time) / Diff;
    return FVector::Lerp(Keys[Lo].Value, Keys[Hi].Value, Alpha);
}

FQuat UAnimInstance::InterpolateKeys(const TArray<FAnimKeyQuat>& Keys, float Time)
{
    if (Keys.empty())
        return FQuat::Identity;
    if (Keys.size() == 1)
        return Keys[0].Value;

    if (Time <= Keys[0].Time)
        return Keys[0].Value;
    if (Time >= Keys.back().Time)
        return Keys.back().Value;

    int32 Lo = 0, Hi = Keys.size() - 1;
    while (Hi - Lo > 1)
    {
        int32 Mid = (Lo + Hi) / 2;
        if (Keys[Mid].Time <= Time)
            Lo = Mid;
        else
            Hi = Mid;
    }

    float Diff = Keys[Hi].Time - Keys[Lo].Time;
    if (Diff < 1e-4f)
        return Keys[Lo].Value;

    float Alpha = (Time - Keys[Lo].Time) / Diff;
    return FQuat::Slerp(Keys[Lo].Value, Keys[Hi].Value, Alpha);
}

void UAnimInstance::BlendPoses(const FSkeletonPose& PoseA, const FSkeletonPose& PoseB, float BlendFactor, FSkeletonPose& OutPose)
{
    const int32 BoneCount = PoseA.LocalTransforms.size();
    OutPose.LocalTransforms.resize(BoneCount);

    for (int32 i = 0; i < BoneCount; ++i)
    {
        const FTransform& A = PoseA.LocalTransforms[i];
        const FTransform& B = PoseB.LocalTransforms[i];

        FVector BlendedPos = FVector::Lerp(A.GetTranslation(), B.GetTranslation(), BlendFactor);
        FQuat BlendedRot = FQuat::Slerp(A.GetRotation(), B.GetRotation(), BlendFactor);
        FVector BlendedScale = FVector::Lerp(A.GetScale3D(), B.GetScale3D(), BlendFactor);

        BlendedRot.Normalize();
        OutPose.LocalTransforms[i] = FTransform(BlendedRot, BlendedPos, BlendedScale);
    }
}
