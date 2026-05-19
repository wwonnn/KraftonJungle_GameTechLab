#pragma once

#include "Core/CoreMinimal.h"
#include "Object/Object.h"
#include "Core/PropertyTypes.h"
#include "Animation/AnimData/AnimNotifyTypes.h"
#include "Reflection/Reflection.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Component/SkinnedMeshComponent.h"
#include "Generated/AnimInstance.generated.h"

class UAnimationStateMachine;
class UAnimInstanceAsset;
class UAnimNotify;
class UAnimNotifyState;
class USkeletalMeshComponent;

// 시간 t에서의 Skeleton Pose (매 프레임 계산)
struct FSkeletonPose
{
    TArray<FTransform> LocalTransforms;
    TArray<FMatrix> ComponentTransforms;
};

UCLASS()
class UAnimInstance : public UObject
{
public:
	GENERATED_BODY()

public:
    ~UAnimInstance() override;

    void Intialize();
    void SetOwningComponent(USkinnedMeshComponent* InOwner);
    USkinnedMeshComponent* GetOwningComponent() const { return Owner; }
    void SetSequence(UAnimSequence* InSequence);
    void SetNextSequence(UAnimSequence* InNext, float InBlendSpeed);
    void SetStateMachine(UAnimationStateMachine* InStateMachine);
    UAnimationStateMachine* GetStateMachine() const { return StateMachine; }
    UAnimationStateMachine* CreateStateMachine();
    bool BuildStateMachineFromAsset(UAnimInstanceAsset* Asset);
    bool PrepareSequenceForPlayback(UAnimSequence* Sequence);
    void SetLooping(bool bInLoop) { bLoop = bInLoop; }
    bool IsLooping() const { return bLoop; }
    bool IsCurrentAnimationFinished() const { return CurrentSequence && !NextSequence && !bLoop && !bPlaying; }
    float GetCurrentTime() const { return CurrentTime; }
    void SetCurrentTime(float InCurrentTime);
    float GetPlayRate() const { return PlayRate; }
    void SetPlayRate(float InPlayRate) { PlayRate = InPlayRate; }
    bool IsPlaying() const { return bPlaying; }
    void Play();
    void Pause();
    void Stop();
    float GetSequenceLength() const;
    bool HasValidSequence() const;
    const TArray<FAnimNotifyEvent>& GetRecentNotifyEvents() const { return RecentNotifyEvents; }

    virtual void UpdateAnimation(float DeltaTime);		// 재생 시간 관리
    virtual void EvaluatePose(FSkeletonPose& OutPose);	// 시간 t에서 Bone의 Pose 계산

protected:
    virtual void NativeInitializeAnimation() {}
    virtual void NativeUpdateAnimation(float DeltaTime) {}
    virtual void NativeAnimNotify(const UAnimSequence* Sequence, const FAnimNotifyEvent& NotifyEvent) {}
    virtual void NativeAnimNotifyBegin(const UAnimSequence* Sequence, const FAnimNotifyEvent& NotifyEvent) {}
    virtual void NativeAnimNotifyTick(const UAnimSequence* Sequence, const FAnimNotifyEvent& NotifyEvent, float DeltaTime) {}
    virtual void NativeAnimNotifyEnd(const UAnimSequence* Sequence, const FAnimNotifyEvent& NotifyEvent) {}

    void InitializeReferencePose(FSkeletonPose& OutPose);
    void EvaluatePoseAtTime(const UAnimSequence* Sequence, float CurrentTime, TArray<FTransform>& OutLocalTransforms);
    FVector InterpolateKeys(const TArray<FVector>& Keys, float Time, float FrameRate);
    FQuat InterpolateKeys(const TArray<FQuat>& Keys, float Time, float FrameRate);
    float GetSequenceLength(const UAnimSequence* Sequence) const;
    float NormalizeTimeForSequence(const UAnimSequence* Sequence, float InTime) const;
    void UpdateRecentNotifyEvents(const UAnimSequence* Sequence, float PreviousTime, float NewTime, float DeltaTime);
    void CollectNotifyEventsCrossed(
        const UAnimSequence* Sequence,
        float PreviousTime,
        float NewTime,
        TArray<FAnimNotifyEvent>& OutEvents) const;
    void DispatchAnimNotify(const UAnimSequence* Sequence, const FAnimNotifyEvent& NotifyEvent);
    void BeginAnimNotifyState(const UAnimSequence* Sequence, const FAnimNotifyEvent& NotifyEvent);
    void TickActiveAnimNotifyStates(const UAnimSequence* Sequence, float PreviousTime, float NewTime, float DeltaTime);
    void EndActiveAnimNotifyState(int32 ActiveStateIndex, const UAnimSequence* Sequence, EAnimNotifyTriggerPhase TriggerPhase);
    void ClearActiveAnimNotifyStates(bool bDispatchEnd);
    UAnimNotify* CreateNotifyObject(const FAnimNotifyEvent& NotifyEvent) const;
    UAnimNotifyState* CreateNotifyStateObject(const FAnimNotifyEvent& NotifyEvent) const;
    bool IsNotifyStateEndCrossed(const FAnimNotifyEvent& NotifyEvent, float PreviousTime, float NewTime, float SequenceLength) const;

	// PoseA와 PoseB를 블렌딩하여 OutPose에 저장 -> Transition 용도
    void BlendPoses(const FSkeletonPose& PoseA, const FSkeletonPose& PoseB, float BlendFactor, FSkeletonPose& OutPose);

    struct FActiveAnimNotifyState
    {
        const UAnimSequence* Sequence = nullptr;
        FAnimNotifyEvent Event;
        UAnimNotifyState* NotifyObject = nullptr;
    };

protected:
    USkinnedMeshComponent* Owner = nullptr;
    UAnimationStateMachine* StateMachine = nullptr;

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

    TArray<FAnimNotifyEvent> RecentNotifyEvents;
    TArray<FActiveAnimNotifyState> ActiveNotifyStates;
};
