#include "MovementStrategies.h"
#include "CollisionSystem.h"
#include <algorithm>

LinearMovement::LinearMovement(FVector direction, float speed, float movementRange)
    : Direction(direction.Normalize())
    , Speed(speed)
    , MovementRange(movementRange)
{}

bool LinearMovement::Update(FVector& outPosition, float& outRotation, const FVector& currentPos, const FVector& playerPos, float dt)
{
    float moveDist = Speed * dt;

    outPosition =
    {
        currentPos.x + Direction.x * moveDist,
        currentPos.y + Direction.y * moveDist
    };
    //outRotation = std::atan2(Direction.y, Direction.x);

    CurrentRange += Direction.x * moveDist;
    if (CurrentRange > MovementRange || CurrentRange < -MovementRange)
    {
        SetDirection(FVector(-Direction.x, -Direction.y));
    }

    // 이동 완료 시 true 반환
    if (false)
        return true;

    return false;
}

void LinearMovement::Reset()
{
    CurrentRange = 0.0f;
}

void LinearMovement::SetDirection(FVector newDir)
{
    Direction = newDir.Normalize();
}

FollowPlayerMovement::FollowPlayerMovement(FVector direction, float speed)
    : Direction(direction.Normalize())
    , Speed(speed)
{
}

bool FollowPlayerMovement::Update(FVector& outPosition, float& outRotation, const FVector& currentPos, const FVector& playerPos, float dt)
{
    float moveDist = Speed * dt;
    Direction = (playerPos - currentPos).Normalize();

    outPosition =
    {
        currentPos.x + Direction.x * moveDist,
        currentPos.y + Direction.y * moveDist
    };
    outRotation = std::atan2(Direction.y, Direction.x) + 3.14159265f / 2.0f;

    return false;
}

void FollowPlayerMovement::Reset()
{
}

void FollowPlayerMovement::SetDirection(FVector newDir)
{
}
