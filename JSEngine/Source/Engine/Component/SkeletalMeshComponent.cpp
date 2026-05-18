#include "SkeletalMeshComponent.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Asset/Skeleton.h"
#include "Core/ResourceManager.h"
#include "Object/ObjectFactory.h"
#include "Render/Proxy/SkeletalMeshRenderProxy.h"

#include <cstring>

DEFINE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)
REGISTER_FACTORY(USkeletalMeshComponent)

namespace
{
bool BindSequenceToMeshSkeleton(USkeletalMesh* SkeletalMesh, UAnimSequence* AnimSequence)
{
    if (!SkeletalMesh || !SkeletalMesh->HasValidMeshData() || !AnimSequence)
    {
        return false;
    }

    USkeleton* MeshSkeleton = SkeletalMesh->GetSkeleton();
    if (!MeshSkeleton || !MeshSkeleton->HasValidSkeletonData())
    {
        return false;
    }

    const FString& MeshSkeletonPath = SkeletalMesh->GetSkeletonAssetPath();
    const FString& SequenceSkeletonPath = AnimSequence->GetSkeletonAssetPath();
    if (!MeshSkeletonPath.empty() &&
        !SequenceSkeletonPath.empty() &&
        MeshSkeletonPath != SequenceSkeletonPath)
    {
        return false;
    }

    AnimSequence->SetSkeleton(MeshSkeleton);
    if (AnimSequence->GetSkeletonAssetPath().empty())
    {
        AnimSequence->SetSkeletonAssetPath(MeshSkeletonPath);
    }

    return true;
}
} // namespace

USkeletalMeshComponent::~USkeletalMeshComponent()
{
    ReleaseSingleNodeAnimation();
}

void USkeletalMeshComponent::Serialize(FArchive& Ar)
{
    USkinnedMeshComponent::Serialize(Ar);

    if (Ar.IsLoading())
    {
        InitializeSingleNodeAnimation();
    }
}

void USkeletalMeshComponent::PostEditProperty(const char* PropertyName)
{
    USkinnedMeshComponent::PostEditProperty(PropertyName);

    if (std::strcmp(PropertyName, "AnimSequencePath") == 0 ||
        std::strcmp(PropertyName, "SkeletalMeshPath") == 0)
    {
        InitializeSingleNodeAnimation();
    }
}

void USkeletalMeshComponent::BeginPlay()
{
    USkinnedMeshComponent::BeginPlay();
    InitializeSingleNodeAnimation();
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    USkinnedMeshComponent::TickComponent(DeltaTime);

    ApplyAnimationPose(DeltaTime);

	// Pose가 바뀐 경우에만 실제 CPU skinning이 수행(dirty flag 이용)
    EnsureSkinningUpdated();
}

void USkeletalMeshComponent::ResetToBindPose()
{
    InitializePoseFromBindPose();
    MarkSkinningDirty();
}

void USkeletalMeshComponent::SetPreviewSequence(UAnimSequence* InSequence)
{
    if (!InSequence)
    {
        ReleaseSingleNodeAnimation();
        ResetToBindPose();
        return;
    }

    if (!BindSequenceToMeshSkeleton(SkeletalMesh, InSequence))
    {
        return;
    }

    EnsureSingleNodeAnimation();
    if (!SingleNodeInstance)
    {
        return;
    }

    SingleNodeInstance->SetSequence(InSequence);
    ApplyAnimationPoseFromInstance(0.0f, false);
}

void USkeletalMeshComponent::SetPreviewLooping(bool bInLooping)
{
    EnsureSingleNodeAnimation();
    if (SingleNodeInstance)
    {
        SingleNodeInstance->SetLooping(bInLooping);
    }
}

void USkeletalMeshComponent::SetPreviewPlaying(bool bInPlaying)
{
    EnsureSingleNodeAnimation();
    if (!SingleNodeInstance)
    {
        return;
    }

    if (bInPlaying)
    {
        SingleNodeInstance->Play();
    }
    else
    {
        SingleNodeInstance->Pause();
    }
}

void USkeletalMeshComponent::SetPreviewPlayRate(float InPlayRate)
{
    EnsureSingleNodeAnimation();
    if (SingleNodeInstance)
    {
        SingleNodeInstance->SetPlayRate(InPlayRate);
    }
}

void USkeletalMeshComponent::SetPreviewTime(float InTime)
{
    EnsureSingleNodeAnimation();
    if (!SingleNodeInstance)
    {
        return;
    }

    SingleNodeInstance->SetCurrentTime(InTime);
    ApplyAnimationPoseFromInstance(0.0f, false);
    EnsureSkinningUpdated();
}

float USkeletalMeshComponent::GetPreviewTime() const
{
    return SingleNodeInstance ? SingleNodeInstance->GetCurrentTime() : 0.0f;
}

float USkeletalMeshComponent::GetPreviewLength() const
{
    return SingleNodeInstance ? SingleNodeInstance->GetSequenceLength() : 0.0f;
}

void USkeletalMeshComponent::TickPreviewAnimation(float DeltaTime)
{
    ApplyAnimationPoseFromInstance(DeltaTime, true);
    EnsureSkinningUpdated();
}

void USkeletalMeshComponent::SetBoneLocalTransform(int32 BoneIndex, const FMatrix& NewLocalTransform)
{
    if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(CurrentLocalPose.size()))
    {
        return;
    }

    CurrentLocalPose[BoneIndex] = NewLocalTransform;
    UpdateCurrentGlobalPose();
    MarkSkinningDirty();
}

const FMatrix& USkeletalMeshComponent::GetBoneLocalTransform(int32 BoneIndex) const
{
	// fallback은 identity
    static const FMatrix Identity = FMatrix::Identity;

    if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(CurrentLocalPose.size()))
    {
        return Identity;
    }

    return CurrentLocalPose[BoneIndex];
}

FMatrix USkeletalMeshComponent::GetBoneGlobalTransform(int32 BoneIndex) const
{
    if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(CurrentGlobalPose.size()))
    {
        return FMatrix::Identity;
    }

    return CurrentGlobalPose[BoneIndex] * GetWorldMatrix();
}

void USkeletalMeshComponent::SetBoneGlobalTransform(int32 BoneIndex, const FMatrix& NewGlobalTransform)
{
    if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(CurrentLocalPose.size()))
    {
        return;
    }

    if (!SkeletalMesh)
    {
        return;
    }

    USkeleton* Skeleton = SkeletalMesh->GetSkeleton();
    if (!Skeleton || !Skeleton->HasValidSkeletonData())
    {
        return;
    }

    const TArray<FBoneInfo>& Bones = Skeleton->GetBones();
    if (BoneIndex >= static_cast<int32>(Bones.size()))
    {
        return;
    }

    const int32 ParentIndex = Bones[BoneIndex].ParentIndex;

    FMatrix ParentGlobalTransform;
    if (ParentIndex >= 0)
    {
        ParentGlobalTransform = CurrentGlobalPose[ParentIndex] * GetWorldMatrix();
    }
    else
    {
        ParentGlobalTransform = GetWorldMatrix();
    }

    // Local = Global * ParentGlobal.Inverse
    const FMatrix NewLocalTransform = NewGlobalTransform * ParentGlobalTransform.GetInverse();
    SetBoneLocalTransform(BoneIndex, NewLocalTransform);
}

FPrimitiveRenderProxy* USkeletalMeshComponent::CreateRenderProxy()
{
    FSkeletalMeshRenderProxy* Proxy = new FSkeletalMeshRenderProxy;
	Proxy->SkeletalMeshComp = this;
    return Proxy;
}

void USkeletalMeshComponent::SetAnimSequencePath(const FString& InAnimSequencePath)
{
    if (AnimSequencePath == InAnimSequencePath)
    {
        return;
    }

    AnimSequencePath = InAnimSequencePath;
    InitializeSingleNodeAnimation();
}

void USkeletalMeshComponent::EnsureSingleNodeAnimation()
{
    if (SingleNodeInstance)
    {
        SingleNodeInstance->SetOwningComponent(this);
        return;
    }

    SingleNodeInstance = UObjectManager::Get().CreateObject<UAnimSingleNodeInstance>();
    if (!SingleNodeInstance)
    {
        return;
    }

    SingleNodeInstance->SetOwningComponent(this);
}

void USkeletalMeshComponent::ReleaseSingleNodeAnimation()
{
    if (!SingleNodeInstance)
    {
        return;
    }

    SingleNodeInstance->SetOwningComponent(nullptr);
    SingleNodeInstance->SetSequence(nullptr);
    UObjectManager::Get().DestroyObject(SingleNodeInstance);
    SingleNodeInstance = nullptr;
}

void USkeletalMeshComponent::InitializeSingleNodeAnimation()
{
    ReleaseSingleNodeAnimation();

    if (!SkeletalMesh || !SkeletalMesh->HasValidMeshData() || AnimSequencePath.empty())
    {
        ResetToBindPose();
        return;
    }

    UAnimSequence* AnimSequence = FResourceManager::Get().LoadAnimSequence(AnimSequencePath);
    if (!BindSequenceToMeshSkeleton(SkeletalMesh, AnimSequence))
    {
        ResetToBindPose();
        return;
    }

    EnsureSingleNodeAnimation();
    if (!SingleNodeInstance)
    {
        ResetToBindPose();
        return;
    }

    SingleNodeInstance->SetSequence(AnimSequence);
    SingleNodeInstance->SetLooping(true);
    ApplyAnimationPoseFromInstance(0.0f, false);
}

void USkeletalMeshComponent::ApplyAnimationPose(float DeltaTime)
{
    ApplyAnimationPoseFromInstance(DeltaTime, true);
}

void USkeletalMeshComponent::ApplyAnimationPoseFromInstance(float DeltaTime, bool bAdvanceTime)
{
    if (!SingleNodeInstance)
    {
        return;
    }

    if (bAdvanceTime)
    {
        SingleNodeInstance->UpdateAnimation(DeltaTime);
    }

    FSkeletonPose Pose;
    SingleNodeInstance->EvaluatePose(Pose);
    ApplyEvaluatedPose(Pose);
}

void USkeletalMeshComponent::ApplyEvaluatedPose(const FSkeletonPose& Pose)
{
    if (Pose.LocalTransforms.empty())
    {
        return;
    }

    CurrentLocalPose.resize(Pose.LocalTransforms.size());
    for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(Pose.LocalTransforms.size()); ++BoneIndex)
    {
        CurrentLocalPose[BoneIndex] = Pose.LocalTransforms[BoneIndex].ToMatrixWithScale();
    }

    MarkSkinningDirty();
}
