#pragma once

#include "Animation/AnimSingleNodeInstance.h"
#include "Component/SkinnedMeshComponent.h"

/**
 * @brief Unreal Engine 스타일에서는 skinned mesh가 skeleton을 이용하는 mesh를 표현하고,
 *        skeletal mesh는 실제로 actor에 붙어서 애니메이션을 붙일 수 있는 component로 사용되고 있으므로
 *        USkeletalMeshComponent 또한 해당 방식대로 우선은 얇게 유지.
 *        핵심 로직들은 대부분 USkinnedMeshComponent로 옮겼습니다.
 */
class USkeletalMeshComponent : public USkinnedMeshComponent
{
public:
    DECLARE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)

    USkeletalMeshComponent() = default;
    ~USkeletalMeshComponent() override = default;

    void BeginPlay() override;
    void TickComponent(float DeltaTime) override;

    EPrimitiveType GetPrimitiveType() const override { return EPrimitiveType::EPT_SkeletalMesh; }

    void ResetToBindPose();
    void SetPreviewSequence(UAnimSequence* InSequence);
    void SetPreviewLooping(bool bInLooping);
    void SetPreviewPlaying(bool bInPlaying);
    void SetPreviewPlayRate(float InPlayRate);
    void SetPreviewTime(float InTime);
    float GetPreviewTime() const;
    float GetPreviewLength() const;
    UAnimSingleNodeInstance* GetPreviewAnimInstance() const { return SingleNodeInstance; }
    void TickPreviewAnimation(float DeltaTime);

    void SetBoneLocalTransform(int32 BoneIndex, const FMatrix& NewLocalTransform);
    const FMatrix& GetBoneLocalTransform(int32 BoneIndex) const;

    FMatrix GetBoneGlobalTransform(int32 BoneIndex) const;
    void SetBoneGlobalTransform(int32 BoneIndex, const FMatrix& NewGlobalTransform);

private:
    void EnsureSingleNodeAnimation();
    void InitializeSingleNodeAnimation();
    void ApplyAnimationPose(float DeltaTime);
    void ApplyAnimationPoseFromInstance(float DeltaTime, bool bAdvanceTime);
    void ApplyEvaluatedPose(const FSkeletonPose& Pose);

private:
    UPROPERTY(EditAnywhere, DisplayName = "AnimInstance")
    UAnimSingleNodeInstance* SingleNodeInstance = nullptr;
};
