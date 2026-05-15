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
    virtual void UpdateAnimation(float DeltaTime);	// 재생 시간 관리
    virtual void EvaluatePose(FSkeletonPose& OutPose);	// 시간 t에서 Bone의 Pose 계산

protected:
    void InitializeReferencePose();
    void EvaluatePoseAtTime(float DeltaTime, TArray<FTransform>& OutLocalTransforms);

	// PoseA와 PoseB를 블렌딩하여 OutPose에 저장 -> Transition 용도
    void BlendPoses(const FSkeletonPose& PoseA, const FSkeletonPose& PoseB, float BlendFactor, FSkeletonPose& OutPose);

	// TODO
	// 1. 재생 시간 관리
	// 2. Evaluate Pose (t에서 각 Bone의 Transform 계산)
	// 3. Animation Blend
	// 4. Animation Notify
	// 5. Animation State Machine

private:
    USkinnedMeshComponent* Owner = nullptr;

    UAnimSequence* CurrSequence = nullptr;
    UAnimSequence* NextSequence = nullptr;

	float CurrentTime = 0.0f;
    float PlayRate = 1.0f;
    bool bLoop = true;
    bool bPlaying = true;
    float BlendFactor = 0.0f; // 0.0f: CurrSequence, 1.0f: NextSequence
};
