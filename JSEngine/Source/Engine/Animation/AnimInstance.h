#pragma once

#include "Core/CoreMinimal.h"
#include "Object/Object.h"
#include "Core/PropertyTypes.h"
#include "Animation/AnimSequence.h"
#include "Component/SkinnedMeshComponent.h"

// 시간 t에서의 Skeleton Pose (매 프레임 계산)
struct FSkeletonPose
{
    TArray<FTransform> LocalTransforms;
    TArray<FMatrix> ComponentTransforms;
};

class UAnimInstance : public UObject
{
public:
	DECLARE_CLASS(UAnimInstance, UObject)

public:
    void SetSequence(UAnimSequence* InSequence);
    void SetNextSequence(UAnimSequence* InNext, float InBlendSpeed);

    virtual void UpdateAnimation(float DeltaTime);		// 재생 시간 관리
    virtual void EvaluatePose(FSkeletonPose& OutPose);	// 시간 t에서 Bone의 Pose 계산

protected:
    void InitializeReferencePose(FSkeletonPose& OutPose);
    void EvaluatePoseAtTime(const UAnimSequence* Sequence, float CurrentTime, TArray<FTransform>& OutLocalTransforms);
    FVector InterpolateKeys(const TArray<FAnimKeyVector>& Keys, float Time);
    FQuat InterpolateKeys(const TArray<FAnimKeyQuat>& Keys, float Time);

	// PoseA와 PoseB를 블렌딩하여 OutPose에 저장 -> Transition 용도
    void BlendPoses(const FSkeletonPose& PoseA, const FSkeletonPose& PoseB, float BlendFactor, FSkeletonPose& OutPose);

	// TODO
	// Animation Notify
	// Animation State Machine

protected:
    USkinnedMeshComponent* Owner = nullptr;

    UAnimSequence* CurrentSequence = nullptr;
    UAnimSequence* NextSequence = nullptr;

	UPROPERTY(VisibleAnywhere, Transient)
	float CurrentTime = 0.0f;
    UPROPERTY(VisibleAnywhere, Transient)
    float NextTime = 0.0f;

    UPROPERTY(EditAnywhere, Min = 0.0, Max = 1.0, Speed = 0.01)
    float PlayRate = 1.0f;
    UPROPERTY(EditAnywhere)
    bool bLoop = true;
    bool bPlaying = true;

	UPROPERTY(VisibleAnywhere, Transient)
    float BlendFactor = 0.0f; // 0.0f: CurrSequence, 1.0f: NextSequence
    UPROPERTY(EditAnywhere, Min = 0.0, Max = 1.0, Speed = 0.01)
	float BlendSpeed = 1.0f;
};
