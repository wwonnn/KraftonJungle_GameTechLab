#include "SkeletalMeshComponent.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Asset/Skeleton.h"
#include "Core/ResourceManager.h"
#include "Object/ObjectFactory.h"

#include <cstring>

DEFINE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)
REGISTER_FACTORY(USkeletalMeshComponent)

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

    int32 ParentIndex = Bones[BoneIndex].ParentIndex;

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
    FMatrix NewLocalTransform = NewGlobalTransform * ParentGlobalTransform.GetInverse();
    SetBoneLocalTransform(BoneIndex, NewLocalTransform);
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

void USkeletalMeshComponent::InitializeSingleNodeAnimation()
{
    SingleNodeInstance = nullptr;

    if (!SkeletalMesh || !SkeletalMesh->HasValidMeshData() || AnimSequencePath.empty())
    {
        return;
    }

    UAnimSequence* AnimSequence = FResourceManager::Get().LoadAnimSequence(AnimSequencePath);
    if (!AnimSequence)
    {
        return;
    }

    USkeleton* MeshSkeleton = SkeletalMesh->GetSkeleton();
    if (!MeshSkeleton || !MeshSkeleton->HasValidSkeletonData())
    {
        return;
    }

    const FString& MeshSkeletonPath = SkeletalMesh->GetSkeletonAssetPath();
    const FString& SequenceSkeletonPath = AnimSequence->GetSkeletonAssetPath();
    if (!MeshSkeletonPath.empty() &&
        !SequenceSkeletonPath.empty() &&
        MeshSkeletonPath != SequenceSkeletonPath)
    {
        return;
    }

    AnimSequence->SetSkeleton(MeshSkeleton);
    if (AnimSequence->GetSkeletonAssetPath().empty())
    {
        AnimSequence->SetSkeletonAssetPath(MeshSkeletonPath);
    }

    SingleNodeInstance = UObjectManager::Get().CreateObject<UAnimSingleNodeInstance>();
    if (!SingleNodeInstance)
    {
        return;
    }

    SingleNodeInstance->SetOwningComponent(this);
    SingleNodeInstance->SetSequence(AnimSequence);
    SingleNodeInstance->SetLooping(true);
}

void USkeletalMeshComponent::ApplyAnimationPose(float DeltaTime)
{
    if (!SingleNodeInstance)
    {
        return;
    }

    SingleNodeInstance->UpdateAnimation(DeltaTime);

    FSkeletonPose Pose;
    SingleNodeInstance->EvaluatePose(Pose);
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
