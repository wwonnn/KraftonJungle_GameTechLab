#include "AnimInstance.h"

#include "Asset/Skeleton.h"
#include "Animation/AnimInstanceAsset.h"
#include "Animation/AnimationStateMachine.h"
#include "Core/Logging/Log.h"
#include "Core/Paths.h"
#include "Core/ResourceManager.h"
#include "Core/Logging/Stats.h"
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

void UAnimInstance::Intialize()
{
	NativeInitializeAnimation();

    if (StateMachine && StateMachine->GetCurrentSequence() && !CurrentSequence)
    {
        SetSequence(StateMachine->GetCurrentSequence());
        SetLooping(StateMachine->GetCurrentStateLooping());
        PlayRate = StateMachine->GetCurrentStatePlayRate();
    }
}

void UAnimInstance::SetOwningComponent(USkinnedMeshComponent* InOwner)
{
    Owner = InOwner;
}

void UAnimInstance::SetSequence(UAnimSequence* InSequence)
{
    if (!PrepareSequenceForPlayback(InSequence))
    {
        return;
    }

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
    if (!InNext || InNext == CurrentSequence || !PrepareSequenceForPlayback(InNext))
    {
        return;
    }

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

void UAnimInstance::SetStateMachine(UAnimationStateMachine* InStateMachine)
{
    StateMachine = InStateMachine;
    if (StateMachine)
    {
        StateMachine->SetOwningAnimInstance(this);
    }
}

UAnimationStateMachine* UAnimInstance::CreateStateMachine()
{
    UAnimationStateMachine* NewStateMachine = UObjectManager::Get().CreateObject<UAnimationStateMachine>();
    SetStateMachine(NewStateMachine);
    return NewStateMachine;
}

bool UAnimInstance::PrepareSequenceForPlayback(UAnimSequence* Sequence)
{
    if (!Sequence)
    {
        return false;
    }

    if (!Owner)
    {
        return true;
    }

    const USkeletalMesh* Mesh = Owner->GetSkeletalMesh();
    if (!Mesh || !Mesh->HasValidMeshData())
    {
        return false;
    }

    USkeleton* MeshSkeleton = Mesh->GetSkeleton();
    if (!MeshSkeleton || !MeshSkeleton->HasValidSkeletonData())
    {
        return false;
    }

    const FString MeshSkeletonPath = FPaths::Normalize(Mesh->GetSkeletonAssetPath());
    const FString SequenceSkeletonPath = FPaths::Normalize(Sequence->GetSkeletonAssetPath());
    if (!MeshSkeletonPath.empty() &&
        !SequenceSkeletonPath.empty() &&
        MeshSkeletonPath != SequenceSkeletonPath)
    {
        UE_LOG_WARNING("[AnimInstance] Sequence skeleton mismatch | Sequence=%s | MeshSkeleton=%s | SequenceSkeleton=%s",
            Sequence->GetName().c_str(),
            MeshSkeletonPath.c_str(),
            SequenceSkeletonPath.c_str());
        return false;
    }

    Sequence->SetSkeleton(MeshSkeleton);
    if (Sequence->GetSkeletonAssetPath().empty())
    {
        Sequence->SetSkeletonAssetPath(MeshSkeletonPath);
    }

    return true;
}

bool UAnimInstance::BuildStateMachineFromAsset(UAnimInstanceAsset* Asset)
{
    if (!Asset)
    {
        return false;
    }

    UAnimationStateMachine* NewStateMachine = CreateStateMachine();
    if (!NewStateMachine)
    {
        return false;
    }

    for (const FAnimInstanceParameterAssetData& Param : Asset->Parameters)
    {
        switch (Param.Type)
        {
        case EAnimStateParameterType::Bool:
            NewStateMachine->RegisterParameterBool(Param.Name, Param.BoolDefault);
            break;
        case EAnimStateParameterType::Float:
            NewStateMachine->RegisterParameterFloat(Param.Name, Param.FloatDefault);
            break;
        case EAnimStateParameterType::Int:
            NewStateMachine->RegisterParameterInt(Param.Name, Param.IntDefault);
            break;
        case EAnimStateParameterType::Trigger:
            NewStateMachine->RegisterParameterTrigger(Param.Name);
            break;
        default:
            break;
        }
    }

    bool bAddedAnyState = false;
    for (const FAnimInstanceStateAssetData& State : Asset->States)
    {
        UAnimSequence* Sequence = FResourceManager::Get().LoadAnimSequence(State.AnimSequencePath);
        if (PrepareSequenceForPlayback(Sequence) &&
            NewStateMachine->AddState(State.Name, Sequence, State.bLoop, State.PlayRate))
        {
            bAddedAnyState = true;
        }
    }

    if (!bAddedAnyState)
    {
        return false;
    }

    if (!NewStateMachine->SetEntryState(Asset->EntryState) && !Asset->States.empty())
    {
        NewStateMachine->SetEntryState(Asset->States[0].Name);
    }

    for (const FAnimInstanceTransitionAssetData& Transition : Asset->Transitions)
    {
        switch (Transition.ConditionType)
        {
        case EAnimTransitionConditionType::BoolEquals:
            NewStateMachine->AddBoolEqualsTransition(
                Transition.FromState,
                Transition.ToState,
                Transition.ParameterName,
                Transition.ExpectedBool,
                Transition.BlendSpeed,
                Transition.Priority);
            break;
        case EAnimTransitionConditionType::FloatGreater:
            NewStateMachine->AddFloatGreaterTransition(
                Transition.FromState,
                Transition.ToState,
                Transition.ParameterName,
                Transition.CompareFloat,
                Transition.BlendSpeed,
                Transition.Priority);
            break;
        case EAnimTransitionConditionType::FloatLessEqual:
            NewStateMachine->AddFloatLessEqualTransition(
                Transition.FromState,
                Transition.ToState,
                Transition.ParameterName,
                Transition.CompareFloat,
                Transition.BlendSpeed,
                Transition.Priority);
            break;
        case EAnimTransitionConditionType::IntEquals:
            NewStateMachine->AddIntEqualsTransition(
                Transition.FromState,
                Transition.ToState,
                Transition.ParameterName,
                Transition.ExpectedInt,
                Transition.BlendSpeed,
                Transition.Priority);
            break;
        case EAnimTransitionConditionType::Trigger:
            NewStateMachine->AddTriggerTransition(
                Transition.FromState,
                Transition.ToState,
                Transition.ParameterName,
                Transition.BlendSpeed,
                Transition.Priority);
            break;
        case EAnimTransitionConditionType::StateFinished:
            NewStateMachine->AddStateFinishedTransition(
                Transition.FromState,
                Transition.ToState,
                Transition.BlendSpeed,
                Transition.Priority);
            break;
        case EAnimTransitionConditionType::Native:
        default:
            break;
        }
    }

    if (UAnimSequence* EntrySequence = NewStateMachine->GetCurrentSequence())
    {
        SetSequence(EntrySequence);
        SetLooping(NewStateMachine->GetCurrentStateLooping());
        PlayRate = NewStateMachine->GetCurrentStatePlayRate();
    }

    return true;
}

void UAnimInstance::UpdateAnimation(float DeltaTime)
{
    SCOPE_STAT_ANIM("Animation Update");
    STAT_COUNTER_ANIM("Active Anim Instances", 1);

    if (!bPlaying || !HasValidSequence())
	// 서브 클래스에서 변수 업데이트
	NativeUpdateAnimation(DeltaTime);

	// State Machine Tick 및 Transition 처리
    if (StateMachine)
    {
        StateMachine->Tick(DeltaTime);

        FAnimStateTransitionResult TransitionResult;
        if (StateMachine->ConsumeTransition(TransitionResult))
        {
            SetLooping(TransitionResult.bLoop);
            PlayRate = TransitionResult.PlayRate;

            if (!CurrentSequence || !bPlaying)
            {
                SetSequence(TransitionResult.TargetSequence);
            }
            else
            {
                SetNextSequence(TransitionResult.TargetSequence, TransitionResult.BlendSpeed);
            }
        }
    }

    if (!bPlaying || CurrentSequence == nullptr || !CurrentSequence->DataModel)
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
    SCOPE_STAT_ANIM("Animation Evaluate");

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

    SCOPE_STAT_ANIM("Animation Evaluate Pose At Time");

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
            continue;
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
    SCOPE_STAT_ANIM("Pose Blend");
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
