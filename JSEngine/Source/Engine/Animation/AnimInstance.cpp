#include "AnimInstance.h"

#include "Asset/Skeleton.h"
#include "Object/ObjectFactory.h"

namespace
{
    bool IsLiveObject(const UObject* Object)
    {
        return Object != nullptr && UObjectManager::Get().ContainsObject(Object);
    }

    const UAnimDataModel* GetValidAnimDataModel(const UAnimSequence* Sequence)
    {
        if (!IsLiveObject(Sequence))
        {
            return nullptr;
        }

        const UAnimDataModel* DataModel = Sequence->DataModel;
        return IsLiveObject(DataModel) ? DataModel : nullptr;
    }
}

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

void UAnimInstance::SetCurrentTime(float InCurrentTime)
{
    CurrentTime = NormalizeTimeForSequence(CurrentSequence, InCurrentTime);
    NextTime = NormalizeTimeForSequence(NextSequence, InCurrentTime);
}

void UAnimInstance::Play()
{
    if (HasValidSequence())
    {
        bPlaying = true;
    }
}

void UAnimInstance::Pause()
{
    bPlaying = false;
}

void UAnimInstance::Stop()
{
    CurrentTime = 0.0f;
    NextTime = 0.0f;
    NextSequence = nullptr;
    BlendFactor = 0.0f;
    bPlaying = false;
}

float UAnimInstance::GetSequenceLength() const
{
    return GetSequenceLength(CurrentSequence);
}

bool UAnimInstance::HasValidSequence() const
{
    return GetValidAnimDataModel(CurrentSequence) != nullptr;
}

void UAnimInstance::UpdateAnimation(float DeltaTime)
{
    if (!bPlaying || !HasValidSequence())
    {
        return;
    }

	// Current Sequence Time
    const float Length = GetSequenceLength(CurrentSequence);

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

		if ((PlayRate >= 0.0f && CurrentTime >= Length) || (PlayRate < 0.0f && CurrentTime <= 0.0f))
            bPlaying = false;
    }

	// Next Sequence Time (Blending)
    if (GetValidAnimDataModel(NextSequence))
    {
        const float NextLength = GetSequenceLength(NextSequence);

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

	USkeleton* Skeleton = Mesh->GetSkeleton();
	if (!Skeleton || !Skeleton->HasValidSkeletonData())
		return;

	const TArray<FBoneInfo>& Bones = Skeleton->GetBones();
    const int32 BoneCount = static_cast<int32>(Bones.size());

	OutPose.LocalTransforms.resize(BoneCount);

	for (int32 i = 0; i < BoneCount; i++)
	{
        OutPose.LocalTransforms[i] = FTransform(Bones[i].LocalBindTransform);
    }
}

void UAnimInstance::EvaluatePoseAtTime(const UAnimSequence* Sequence, float CurrentTime, TArray<FTransform>& OutLocalTransforms)
{
    const UAnimDataModel* Model = GetValidAnimDataModel(Sequence);
    if (!Model)
    {
        return;
    }

    if (Owner)
    {
        const USkeletalMesh* Mesh = Owner->GetSkeletalMesh();
        USkeleton* Skeleton = Mesh ? Mesh->GetSkeleton() : nullptr;
        if (Skeleton && Skeleton->HasValidSkeletonData())
        {
            const TArray<FBoneInfo>& Bones = Skeleton->GetBones();
            OutLocalTransforms.resize(Bones.size());
            for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(Bones.size()); ++BoneIndex)
            {
                OutLocalTransforms[BoneIndex] = FTransform(Bones[BoneIndex].LocalBindTransform);
            }
        }
    }

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

float UAnimInstance::GetSequenceLength(const UAnimSequence* Sequence) const
{
    const UAnimDataModel* Model = GetValidAnimDataModel(Sequence);
    if (!Model)
    {
        return 0.0f;
    }

    const float FrameRate = Model->FrameRate.AsDecimal();
    if (FrameRate <= 0.0f || Model->NumberOfKeys <= 1)
    {
        return 0.0f;
    }

    return static_cast<float>(Model->NumberOfKeys - 1) / FrameRate;
}

float UAnimInstance::NormalizeTimeForSequence(const UAnimSequence* Sequence, float InTime) const
{
    const float Length = GetSequenceLength(Sequence);
    if (Length <= 0.0f)
    {
        return 0.0f;
    }

    if (bLoop)
    {
        float WrappedTime = std::fmod(InTime, Length);
        if (WrappedTime < 0.0f)
        {
            WrappedTime += Length;
        }

        return WrappedTime;
    }

    return std::clamp(InTime, 0.0f, Length);
}
