#include "AnimInstance.h"

#include "Object/ObjectFactory.h"

DEFINE_CLASS(UAnimInstance, UObject)
REGISTER_FACTORY(UAnimInstance)

void UAnimInstance::SetOwningComponent(USkinnedMeshComponent* InOwner)
{
    Owner = InOwner;
}

void UAnimInstance::SetSequence(UAnimSequence* InSequence)
{
	CurrentSequence = InSequence;
	CurrentTime = 0.0f;
	bPlaying = true;

	// Next Sequence 초기화
	NextSequence = nullptr;
	NextTime = 0.0f;
	BlendFactor = 0.0f;
}
void UAnimInstance::SetNextSequence(UAnimSequence* InNext, float InBlendSpeed)
{
    NextSequence = InNext;
    NextTime = 0.0f;
    BlendFactor = 0.0f;
    BlendSpeed = InBlendSpeed;
}

void UAnimInstance::UpdateAnimation(float DeltaTime)
{
    if (!bPlaying || CurrentSequence == nullptr || !CurrentSequence->DataModel)
    {
        return;
    }

	// Current Sequence Time
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

	// Next Sequence Time (Blending)
    if (NextSequence && NextSequence->DataModel)
    {
        const UAnimDataModel* Model = NextSequence->DataModel;
        const float NextLength = (float)(Model->NumberOfKeys - 1) / Model->FrameRate.AsDecimal();

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

		// Blend 완료 
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
    if (!Sequence || !Sequence->DataModel)
    {
        return;
    }

    if (Owner && Owner->GetSkeletalMesh() && Owner->GetSkeletalMesh()->HasValidMeshData())
    {
        const TArray<FBoneInfo>& Bones = Owner->GetSkeletalMesh()->GetBones();
        OutLocalTransforms.resize(Bones.size());
        for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(Bones.size()); ++BoneIndex)
        {
            OutLocalTransforms[BoneIndex] = FTransform(Bones[BoneIndex].LocalBindTransform);
        }
    }

    const UAnimDataModel* Model = Sequence->DataModel;
    const float FrameRate = Model->FrameRate.AsDecimal();
    if (FrameRate <= 0.0f)
    {
        return;
    }

	for (int32 i = 0; i < static_cast<int32>(Model->BoneAnimationTracks.size()); ++i)
    {
        const FBoneAnimationTrack& Track = Model->BoneAnimationTracks[i];
        const FRawAnimSequenceTrack& RawTrack = Track.InternalTrackData;
        int32 BoneIndex = Track.BoneTreeIndex;
        if (BoneIndex < 0)
        {
            continue;
        }

        if (BoneIndex >= static_cast<int32>(OutLocalTransforms.size()))
        {
            OutLocalTransforms.resize(BoneIndex + 1);
        }

        FVector Position = InterpolateKeys(RawTrack.PosKeys, CurrentTime, FrameRate);
        FQuat Rotation = InterpolateKeys(RawTrack.RotKeys, CurrentTime, FrameRate);
        FVector Scale = InterpolateKeys(RawTrack.ScaleKeys, CurrentTime, FrameRate);

        Rotation.Normalize(); // 보간 후 정규화 필수

        OutLocalTransforms[BoneIndex] = FTransform(Rotation, Position, Scale);
    }
}

FVector UAnimInstance::InterpolateKeys(const TArray<FVector>& Keys, float Time, float FrameRate)
{
    if (Keys.empty())
        return FVector::ZeroVector;
    if (Keys.size() == 1)
        return Keys[0];

	float KeyIndex = Time * FrameRate;
    int32 Lo = (int32)KeyIndex;
    int32 Hi = Lo + 1;

    // 범위 클램프
    Lo = std::clamp(Lo, 0, (int32)Keys.size() - 1);
    Hi = std::clamp(Hi, 0, (int32)Keys.size() - 1);

    if (Lo == Hi)
        return Keys[Lo];

    float Alpha = KeyIndex - (float)Lo;
    return FVector::Lerp(Keys[Lo], Keys[Hi], Alpha);
}

FQuat UAnimInstance::InterpolateKeys(const TArray<FQuat>& Keys, float Time, float FrameRate)
{
    if (Keys.empty())
        return FQuat::Identity;
    if (Keys.size() == 1)
        return Keys[0];

    float KeyIndex = Time * FrameRate;
    int32 Lo = (int32)KeyIndex;
    int32 Hi = Lo + 1;

    Lo = std::clamp(Lo, 0, (int32)Keys.size() - 1);
    Hi = std::clamp(Hi, 0, (int32)Keys.size() - 1);

    if (Lo == Hi)
        return Keys[Lo];

    float Alpha = KeyIndex - (float)Lo;
    return FQuat::Slerp(Keys[Lo], Keys[Hi], Alpha);
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
