#pragma once

#include "MovementStrategies.h"
#include <vector>
#include <memory>

class MovementSequence : public IMovementStrategy
{
public:
    struct FStrategySlot
    {
        std::unique_ptr<IMovementStrategy> Strategy;
        float Duration;   // 실행 시간
    };
    explicit MovementSequence(bool bLoop = false);

public:
    MovementSequence& Add(std::unique_ptr<IMovementStrategy> strategy,
        float duration = 0.0f);

    bool Update(FVector& outPosition,
        float& outRotation,
        const FVector& currentPos,
        const FVector& playerPos,
        float            dt) override;

    void Reset() override;

    void SetLoop(bool loop) { bLoop = loop; }
    int  GetCurrentIndex() const { return CurrentIndex; }
    bool IsFinished()      const { return bFinished; }

private:
    void AdvanceToNext();

    std::vector<FStrategySlot> Slots;
    int   CurrentIndex = 0;
    float ElapsedInSlot = 0.f;   // 현재 슬롯에서 경과한 시간
    bool  bLoop = false;
    bool  bFinished = false;
};
