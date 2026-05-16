#include "SkeletalMeshComponent.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Asset/Skeleton.h"
#include "Core/ResourceManager.h"
#include "Animation/LuaAnimInstance.h"
#include "Object/ObjectFactory.h"
#include "Render/Proxy/SkeletalMeshRenderProxy.h"
#include "Core/Paths.h"

#include <cstring>

DEFINE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)
REGISTER_FACTORY(USkeletalMeshComponent)

namespace
{
bool IsLiveSingleNodeInstance(const UAnimSingleNodeInstance* Instance)
{
    return Instance != nullptr && UObjectManager::Get().ContainsObject(Instance);
}

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

    const FString MeshSkeletonPath = FPaths::ToProjectRelativePath(SkeletalMesh->GetSkeletonAssetPath());
    const FString SequenceSkeletonPath = FPaths::ToProjectRelativePath(AnimSequence->GetSkeletonAssetPath());
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

UObject* USkeletalMeshComponent::Duplicate()
{
    UObject* DuplicatedObject = FObjectFactory::Get().Create(GetTypeInfo()->name);
    USkeletalMeshComponent* DuplicatedComponent = Cast<USkeletalMeshComponent>(DuplicatedObject);
    if (!DuplicatedComponent)
    {
        return DuplicatedObject;
    }

    DuplicatedComponent->bDeferAnimationInitialization = true;
    DuplicatedComponent->CopyPropertiesFrom(this);
    DuplicatedComponent->bDeferAnimationInitialization = false;
    DuplicatedComponent->SingleNodeInstance = nullptr;
    DuplicatedComponent->PostDuplicate(this);
    return DuplicatedComponent;
}

void USkeletalMeshComponent::Serialize(FArchive& Ar)
{
    USkinnedMeshComponent::Serialize(Ar);

    if (Ar.IsLoading())
    {
        InitializeSingleNodeAnimation();
    }
}

void USkeletalMeshComponent::PostDuplicate(UObject* Original)
{
    USkinnedMeshComponent::PostDuplicate(Original);

    SingleNodeInstance = nullptr;
    InitializeSingleNodeAnimation();
}

void USkeletalMeshComponent::PostEditProperty(const char* PropertyName)
{
    USkinnedMeshComponent::PostEditProperty(PropertyName);

    if (bDeferAnimationInitialization)
    {
        return;
    }

    if (std::strcmp(PropertyName, "AnimSequencePath") == 0 ||
        std::strcmp(PropertyName, "SkeletalMeshPath") == 0)
    {
        InitializeSingleNodeAnimation();
    }
}

void USkeletalMeshComponent::BeginPlay()
{
    USkinnedMeshComponent::BeginPlay();
    InitializeAnimation();
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
    if (!IsLiveSingleNodeInstance(SingleNodeInstance))
    {
        return;
    }

    SingleNodeInstance->SetSequence(InSequence);
    ApplyAnimationPoseFromInstance(0.0f, false);
}

void USkeletalMeshComponent::SetPreviewLooping(bool bInLooping)
{
    EnsureSingleNodeAnimation();
    if (IsLiveSingleNodeInstance(SingleNodeInstance))
    {
        SingleNodeInstance->SetLooping(bInLooping);
    }
}

void USkeletalMeshComponent::SetPreviewPlaying(bool bInPlaying)
{
    EnsureSingleNodeAnimation();
    if (!IsLiveSingleNodeInstance(SingleNodeInstance))
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
    if (IsLiveSingleNodeInstance(SingleNodeInstance))
    {
        SingleNodeInstance->SetPlayRate(InPlayRate);
    }
}

void USkeletalMeshComponent::SetPreviewTime(float InTime)
{
    EnsureSingleNodeAnimation();
    if (!IsLiveSingleNodeInstance(SingleNodeInstance))
    {
        return;
    }

    SingleNodeInstance->SetCurrentTime(InTime);
    ApplyAnimationPoseFromInstance(0.0f, false);
    EnsureSkinningUpdated();
}

float USkeletalMeshComponent::GetPreviewTime() const
{
    return IsLiveSingleNodeInstance(SingleNodeInstance) ? SingleNodeInstance->GetCurrentTime() : 0.0f;
}

float USkeletalMeshComponent::GetPreviewLength() const
{
    return IsLiveSingleNodeInstance(SingleNodeInstance) ? SingleNodeInstance->GetSequenceLength() : 0.0f;
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
    const FString StoredAnimSequencePath = FPaths::ToProjectRelativePath(InAnimSequencePath);
    if (AnimSequencePath == StoredAnimSequencePath)
    {
        return;
    }

    AnimSequencePath = StoredAnimSequencePath;
    InitializeSingleNodeAnimation();
}

void USkeletalMeshComponent::EnsureSingleNodeAnimation()
void USkeletalMeshComponent::SetAnimInstance(UAnimInstance* InAnimInstance)
{
    if (IsLiveSingleNodeInstance(SingleNodeInstance))
    AnimInstance = InAnimInstance;
    if (AnimInstance)
    {
        AnimInstance->SetOwningComponent(this);
        AnimInstance->Intialize();
    }
}

void USkeletalMeshComponent::SetLuaAnimScriptName(const FString& InScriptName)
{
    LuaAnimScriptName = InScriptName;
}

void USkeletalMeshComponent::InitializeAnimation()
{
    if (!SkeletalMesh || !SkeletalMesh->HasValidMeshData())
    {
        SingleNodeInstance->SetOwningComponent(this);
        SingleNodeInstance = nullptr;
        AnimInstance = nullptr;
        return;
    }

    SingleNodeInstance = nullptr;

    SingleNodeInstance = UObjectManager::Get().CreateObject<UAnimSingleNodeInstance>();
    if (!SingleNodeInstance)
    if (!LuaAnimScriptName.empty())
    {
        ULuaAnimInstance* LuaAnimInstance = UObjectManager::Get().CreateObject<ULuaAnimInstance>();
        if (LuaAnimInstance)
        {
            LuaAnimInstance->SetScriptName(LuaAnimScriptName);
            SetAnimInstance(LuaAnimInstance);
        }
        return;
    }

    SingleNodeInstance->SetOwningComponent(this);
}

void USkeletalMeshComponent::ReleaseSingleNodeAnimation()
{
    UAnimSingleNodeInstance* Instance = SingleNodeInstance;
    SingleNodeInstance = nullptr;

    if (!IsLiveSingleNodeInstance(Instance))
    const TArray<UAnimSequence*>& AnimationSequences = SkeletalMesh->GetAnimationSequences();
    if (AnimationSequences.empty() || AnimationSequences[0] == nullptr)
    {
        SingleNodeInstance = nullptr;
        AnimInstance = nullptr;
        return;
    }

    USkeleton* MeshSkeleton = SkeletalMesh->GetSkeleton();
    if (!MeshSkeleton || !MeshSkeleton->HasValidSkeletonData())
    {
        return;
    }

    Instance->SetOwningComponent(nullptr);
    Instance->SetSequence(nullptr);
    UObjectManager::Get().DestroyObject(Instance);
}

void USkeletalMeshComponent::InitializeSingleNodeAnimation()
{
    ReleaseSingleNodeAnimation();

    if (!SkeletalMesh || !SkeletalMesh->HasValidMeshData() || AnimSequencePath.empty())
    {
        ResetToBindPose();
        return;
    }

    AnimSequencePath = FPaths::ToProjectRelativePath(AnimSequencePath);
    UAnimSequence* AnimSequence = FResourceManager::Get().LoadAnimSequence(AnimSequencePath);
    if (!BindSequenceToMeshSkeleton(SkeletalMesh, AnimSequence))
    {
        ResetToBindPose();
        return;
    }

    EnsureSingleNodeAnimation();
    if (!IsLiveSingleNodeInstance(SingleNodeInstance))
    {
        ResetToBindPose();
        return;
    }

    SingleNodeInstance->SetSequence(AnimSequence);
    SingleNodeInstance->SetLooping(true);
    ApplyAnimationPoseFromInstance(0.0f, false);
    SingleNodeInstance->Intialize();
    AnimInstance = SingleNodeInstance;
}

void USkeletalMeshComponent::ApplyAnimationPose(float DeltaTime)
{
    ApplyAnimationPoseFromInstance(DeltaTime, true);
}

void USkeletalMeshComponent::ApplyAnimationPoseFromInstance(float DeltaTime, bool bAdvanceTime)
{
    if (!IsLiveSingleNodeInstance(SingleNodeInstance))
    if (!AnimInstance)
    {
        return;
    }

    if (bAdvanceTime)
    {
        SingleNodeInstance->UpdateAnimation(DeltaTime);
    }
    AnimInstance->UpdateAnimation(DeltaTime);

    FSkeletonPose Pose;
    SingleNodeInstance->EvaluatePose(Pose);
    ApplyEvaluatedPose(Pose);
}

void USkeletalMeshComponent::ApplyEvaluatedPose(const FSkeletonPose& Pose)
{
    AnimInstance->EvaluatePose(Pose);
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
