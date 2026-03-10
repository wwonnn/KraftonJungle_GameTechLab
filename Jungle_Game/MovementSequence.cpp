#include "MovementSequence.h"

MovementSequence::MovementSequence(bool bLoop)
    : bLoop(bLoop)
{
}

MovementSequence& MovementSequence::Add(std::unique_ptr<IMovementStrategy> strategy, float duration)
{
    Slots.push_back({ std::move(strategy), duration });
    return *this;
}

bool MovementSequence::Update(FVector& outPosition, float& outRotation, const FVector& currentPos, const FVector& playerPos, float dt)
{
    if(Slots.empty() || bFinished)
        return true;

    float remainingDt = dt;
    while (remainingDt > 0.f)
    {
        if (CurrentIndex >= static_cast<int>(Slots.size()))
        {
            // 한 사이클 끝났을 때
            if (bLoop) Reset();
            else { bFinished = true; return true; }
        }

        FStrategySlot& slot = Slots[CurrentIndex];
        if (!slot.Strategy) { AdvanceToNext(); continue; }

        // Duration > 0
        if (slot.Duration > 0.f)
        {
            float timeLeft = slot.Duration - ElapsedInSlot;

            // 이번 프레임이 슬롯 잔여 시간 안에 끝나는 경우
            float consumeDt = (remainingDt <= timeLeft)
                ? remainingDt
                : timeLeft;

            slot.Strategy->Update(outPosition, outRotation,
                currentPos, playerPos, consumeDt);

            ElapsedInSlot += consumeDt;
            remainingDt -= consumeDt;

            if (ElapsedInSlot >= slot.Duration)
                AdvanceToNext();  
        }
        else
        {
            // Duration <= 0: 완료 실행 전까지 무한 실행 슬롯
            bool done = slot.Strategy->Update(outPosition, outRotation,
                currentPos, playerPos, remainingDt);
            remainingDt = 0.f;  

            if (done) AdvanceToNext();
        }
    }

    return false;
}

void MovementSequence::Reset()
{
    CurrentIndex = 0;
    ElapsedInSlot = 0.f;
    bFinished = false;

    for (auto& slot : Slots)
        if (slot.Strategy) slot.Strategy->Reset();
}

void MovementSequence::AdvanceToNext()
{
    ++CurrentIndex;
    ElapsedInSlot = 0.f;

    // 다음 슬롯의 전략을 초기 상태로 준비
    if (CurrentIndex < static_cast<int>(Slots.size()))
    {
        if (Slots[CurrentIndex].Strategy)
            Slots[CurrentIndex].Strategy->Reset();
    }
}
