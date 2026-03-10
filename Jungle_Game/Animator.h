#pragma once
#include <string>

struct FAnimationClip {
    std::string TextureName;   // 사용할 스프라이트 시트 이름
    int FrameCount;            // 총 프레임 수
    int Columns;               // 시트의 가로 프레임 수
    int Rows;                  // 시트의 세로 프레임 수
    float FrameDuration;       // 프레임당 시간 (초)
    bool bLoop;
};

struct FAnimatorState {
    FAnimationClip CurrentClip;
    int CurrentFrame = 0;
    float ElapsedTime = 0.f;
};

class UAnimator
{
public:
    explicit UAnimator();

public:
    void SetAnimationClip(FAnimationClip& Clip);
    void Update(float DeltaTime, FAnimatorState& State);
    void GetFrameUV(
        float& OutU,
        float& OutV,
        float& OutScaleU,
        float& OutScaleV);

public:
    FAnimatorState State;
};
