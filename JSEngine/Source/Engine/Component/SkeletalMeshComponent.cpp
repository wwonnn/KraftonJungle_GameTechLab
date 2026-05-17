#include "SkeletalMeshComponent.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Object/ObjectFactory.h"
#include "Render/Proxy/SkeletalMeshRenderProxy.h"

DEFINE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)
REGISTER_FACTORY(USkeletalMeshComponent)

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

    const TArray<FBoneInfo>& Bones = SkeletalMesh->GetBones();
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

FPrimitiveRenderProxy* USkeletalMeshComponent::CreateRenderProxy()
{
    FSkeletalMeshRenderProxy* Proxy = new FSkeletalMeshRenderProxy;
	Proxy->SkeletalMeshComp = this;
    return Proxy;
}

void USkeletalMeshComponent::InitializeSingleNodeAnimation()
{
    if (!SkeletalMesh || !SkeletalMesh->HasValidMeshData())
    {
        SingleNodeInstance = nullptr;
        return;
    }

    const TArray<UAnimSequence*>& AnimationSequences = SkeletalMesh->GetAnimationSequences();
    if (AnimationSequences.empty() || AnimationSequences[0] == nullptr)
    {
        SingleNodeInstance = nullptr;
        return;
    }

    SingleNodeInstance = UObjectManager::Get().CreateObject<UAnimSingleNodeInstance>();
    if (!SingleNodeInstance)
    {
        return;
    }

    SingleNodeInstance->SetOwningComponent(this);
    SingleNodeInstance->SetSequence(AnimationSequences[0]);
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
