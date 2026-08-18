#include "Animator.h"

UAnimator::UAnimator() {

}

void UAnimator::SetAnimationClip(FAnimationClip& Clip)
{
    State.CurrentClip = Clip;
    State.CurrentFrame = 0;
    State.ElapsedTime = 0.f;
}

void UAnimator::Update(float DeltaTime, FAnimatorState& State)
{
    if (State.CurrentClip.FrameCount <= 0) return;

    State.ElapsedTime += DeltaTime;

    if (State.ElapsedTime >= State.CurrentClip.FrameDuration) {
        State.ElapsedTime -= State.CurrentClip.FrameDuration;
        State.CurrentFrame++;

        if (State.CurrentFrame >= State.CurrentClip.FrameCount) {
            State.CurrentFrame = State.CurrentClip.bLoop ? 0 : State.CurrentClip.FrameCount - 1;
        }
    }
}

void UAnimator::GetFrameUV(float& OutU, float& OutV, float& OutScaleU, float& OutScaleV)
{
    if (State.CurrentClip.FrameCount <= 0) return;

    FAnimationClip clip = State.CurrentClip;

    OutScaleU = 1.f / clip.Columns;
    OutScaleV = 1.f / clip.Rows;

    int Col = State.CurrentFrame % clip.Columns;
    int Row = State.CurrentFrame / clip.Columns;

    OutU = Col * OutScaleU;
    OutV = Row * OutScaleV;
}
