#include "MovementStrategies.h"
#include "CollisionSystem.h"
#include <algorithm>

LinearMovement::LinearMovement(FVector direction, float speed)
    : Direction(direction.Normalize())
    , Speed(speed)
{}

bool LinearMovement::Update(FVector& outPosition, float& outRotation, const FVector& currentPos, const FVector& playerPos, float dt)
{
    float moveDist = Speed * dt;

    outPosition =
    {
        currentPos.x + Direction.x * moveDist,
        currentPos.y + Direction.y * moveDist
    };
    outRotation = std::atan2(Direction.y, Direction.x);

    // 벽 충돌 시 방향 바꿈
    FVector box(1.0f, 1.0f, 1.0f);
    if (UCollisionSystem::Get().CheckWallCollision(outPosition, box)) {
        SetDirection(FVector(-Direction.x, -Direction.y));
    }

    // 이동 불가 조건 시 false 반환
    if (false)
        return false;

    return true;
}

void LinearMovement::Reset()
{
}

void LinearMovement::SetDirection(FVector newDir)
{
    Direction = newDir.Normalize();
}
