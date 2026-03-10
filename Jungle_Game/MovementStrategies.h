#pragma once
#include "Math.h"
#include <cmath>

class IMovementStrategy
{
public:
    virtual ~IMovementStrategy() = default;

    /**
     * @param outPosition  이번 프레임에 적용할 월드 위치 (출력)
     * @param outRotation  이번 프레임에 적용할 Z 회전값 rad (출력)
     * @param currentPos   현재 위치 (입력)
     * @param playerPos    플레이어 현재 위치 (입력, 필요한 전략만 사용)
     * @param dt           DeltaTime
     * @return             true = 이동 완료
     */
    virtual bool Update(FVector& outPosition,
        float& outRotation,
        const FVector& currentPos,
        const FVector& playerPos,
        float            dt) = 0;

    virtual void Reset() = 0;
};

// ------------------------------------------------------------

/// <summary>
/// x축 직선 이동 패턴
/// </summary>
class LinearMovement : public IMovementStrategy
{
public:
    explicit LinearMovement(FVector direction = { 1.0f, 0.0f }, float speed = 1.0f, float movementRange = 0.3f);

    bool Update(FVector& outPosition,
        float& outRotation,
        const FVector& currentPos,
        const FVector& playerPos,
        float            dt) override;

    void Reset() override;
    void SetDirection(FVector newDir);

private:
    FVector Direction;       
    float Speed;
    float MovementRange;    // 이동 범위
    float CurrentRange = 0.0f; // 현재 이동한 거리
};

/// <summary>
/// 플레이어 방향으로 이동 패턴
/// </summary>
class FollowPlayerMovement : public IMovementStrategy
{
public:
    explicit FollowPlayerMovement(FVector direction = { 1.0f, 0.0f }, float speed = 1.0f);

    bool Update(FVector& outPosition,
        float& outRotation,
        const FVector& currentPos,
        const FVector& playerPos,
        float            dt) override;

    void Reset() override;
    void SetDirection(FVector newDir);

private:
    FVector Direction;
    float Speed;
};
