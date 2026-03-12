#include "MovementStrategies.h"
#include "CollisionSystem.h"
#include "Random.h"
#include <algorithm>
#include <string>

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
    outRotation = 3.14159265f; // look down

    CurrentRange += moveDist;
    if (abs(CurrentRange) > MovementRange)
    {
        CurrentRange *= -1.0f;
        if (CurrentRange < -MovementRange)
            CurrentRange = -MovementRange;
        else if (CurrentRange > MovementRange)
            CurrentRange = MovementRange;

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
    outRotation = std::atan2(Direction.y, Direction.x) - 3.14159265f / 2.0f;

    return false;
}

void FollowPlayerMovement::Reset()
{
}

void FollowPlayerMovement::SetDirection(FVector newDir)
{
}

ZigZagMovement::ZigZagMovement(FVector direction, float speed, float zigzagDistance, float zigzagAngle)
    : Direction(direction.Normalize())
    , Speed(speed)
    , ZigzagDistance(zigzagDistance)
    , ZigzagAngle(zigzagAngle)
    , CurrentDirection(direction.Normalize())
{}

bool ZigZagMovement::Update(FVector& outPosition, float& outRotation, const FVector& currentPos, const FVector& playerPos, float dt)
{
    float moveDist = Speed * dt;

    outPosition = FVector(
        currentPos.x + CurrentDirection.x * moveDist,
        currentPos.y + CurrentDirection.y * moveDist
    );
    outRotation = std::atan2(CurrentDirection.y, CurrentDirection.x) - 3.14159265f / 2.0f;

    CurrentDistance += moveDist;

    if (CurrentDistance >= ZigzagDistance)
    {
        CurrentDistance = 0.0f;

        float angle = (ZigzagState == 0) ? -ZigzagAngle : ZigzagAngle;

        float cosAngle = std::cos(angle);
        float sinAngle = std::sin(angle);

        CurrentDirection = FVector(
            Direction.x * cosAngle - Direction.y * sinAngle,
            Direction.x * sinAngle + Direction.y * cosAngle
        ).Normalize();

        ZigzagState = 1 - ZigzagState;
    }

    if (outPosition.y < -1.0f)
    {
        outPosition.x = rand() % 100 / 100.0f * 1.5f - 0.75f;
        outPosition.y = 1.0f;
    }

    return false;
}

void ZigZagMovement::Reset()
{
    CurrentDistance = 0.0f;
    ZigzagState = 0;
    CurrentDirection = Direction;
}

void ZigZagMovement::SetDirection(FVector newDir)
{
    Direction = newDir.Normalize();
    CurrentDirection = Direction;
}

CircularMovement::CircularMovement(FVector direction, float speed, float circleRadius, float circleSpeed, float circleDistance)
    : Direction(direction)
    , CurrentDirection(direction)
    , Speed(speed)
    , CircleRadius(circleRadius)
    , CircleSpeed(circleSpeed)
    , CircleDistance(circleDistance)
    , CircleAngle(0.0f)
    , bInCircle(false)
{
}

bool CircularMovement::Update(FVector& outPosition, float& outRotation, const FVector& currentPos, const FVector& playerPos, float dt)
{
    if (!bInCircle)
    {
        outPosition = currentPos + CurrentDirection * Speed * dt;
        outRotation = std::atan2(CurrentDirection.y, CurrentDirection.x) - 3.14159265f / 2.0f;

        CurrentDistance += Speed * dt;
        if (CurrentDistance >= CircleDistance)
        {
            bInCircle = true;
            CircleAngle = 0.0f;
            CurrentDistance = 0.0f;
        }
    }
    else
    {
        // 원 그리기
        CircleAngle += CircleSpeed * dt;
        
        float offsetX = CircleRadius * std::cos(CircleAngle);
        float offsetY = CircleRadius * std::sin(CircleAngle);
        
        outPosition.x = currentPos.x + offsetX;
        outPosition.y = currentPos.y + offsetY;

        outRotation = CircleAngle + 1.5708f;
        
        // 원을 한 바퀴 돌면 직선 이동으로 전환
        if (CircleAngle >= 6.28318f) // 2 * PI
        {
            bInCircle = false;
            CircleAngle = 0.0f;

            float angle = 3.1415926535f / 6.0f;
            float cosAngle = std::cos(angle);
            float sinAngle = std::sin(angle);

            CurrentDirection = FVector(
                Direction.x * cosAngle - Direction.y * sinAngle,
                Direction.x * sinAngle + Direction.y * cosAngle
            ).Normalize();
        }
    }

    if (outPosition.y < -1.0f)
    {
        outPosition.y = 1.0f;
    }
    if (outPosition.x < -1.0f)
    {
        outPosition.x = 1.0f;
    }
    if (outPosition.x > 1.0f)
    {
        outPosition.x = -1.0f;
    }
    
    return false;
}

void CircularMovement::Reset()
{
    CircleAngle = 0.0f;
    bInCircle = false;
}

void CircularMovement::SetDirection(FVector newDir)
{
    Direction = newDir;
}

DiveToPlayerMovement::DiveToPlayerMovement(float speed)
    : Speed(speed)
{
}

bool DiveToPlayerMovement::Update(FVector& outPosition, float& outRotation, const FVector& currentPos, const FVector& playerPos, float dt)
{
    if (!bInit) {
        bInit = true;
        StartPos = currentPos;
        PlayerPos = playerPos;
    }

    if (MoveDist < 1.0f && !bBack) {
        MoveDist += Speed * dt;
        outPosition = StartPos + (PlayerPos - StartPos) * MoveDist;
    }
    else {
        // 복귀
        if (!bBack) {
            bBack = true;
        }
        MoveDist -= Speed * dt;
        if (MoveDist <= 0.0f)
            MoveDist = 0.0f;

        outPosition = StartPos + (PlayerPos - StartPos) * (MoveDist);
    }

    return false;
}

void DiveToPlayerMovement::Reset()
{
    MoveDist = 0.0f;
    bInit = false;
    bBack = false;
}

void DiveToPlayerMovement::SetDirection(FVector newDir)
{
}

ItemZigZagMovement::ItemZigZagMovement(FVector direction, float speed, float amplitude, float frequency, float elapsedTime)
    : Direction(direction.Normalize())
    , Speed(speed)
    , Amplitude(amplitude)
    , Frequency(frequency)
    , ElapsedTime(elapsedTime)
{}

bool ItemZigZagMovement::Update(FVector& outPosition, float& outRotation, const FVector& currentPos, const FVector& playerPos, float dt)
{
    ElapsedTime += dt;

    FVector baseMove = Direction * Speed * dt;

    FVector sideDir = FVector(-Direction.y, Direction.x);
    float horizontalOffset = std::sin(ElapsedTime * Frequency) * Amplitude;

    float waveVariation = std::cos(ElapsedTime * Frequency) * Amplitude * Frequency * dt;

    outPosition.x = currentPos.x + baseMove.x + (sideDir.x * waveVariation);
    outPosition.y = currentPos.y + baseMove.y + (sideDir.y * waveVariation);

    outRotation = 0.0f;

    return false;
}

void ItemZigZagMovement::Reset()
{
    ElapsedTime = 0.0f;
}

ItemLinearMovement::ItemLinearMovement(FVector direction, float speed)
    : Direction(direction.Normalize())
    , Speed(speed)
{
}

bool ItemLinearMovement::Update(FVector& outPosition, float& outRotation, const FVector& currentPos, const FVector& playerPos, float dt)
{
    float moveDist = Speed * dt;

    outPosition.x = currentPos.x + Direction.x * moveDist;
    outPosition.y = currentPos.y + Direction.y * moveDist;

    bool bReflected = false;
    const float Limit = 0.98f;

    if (outPosition.x >= Limit || outPosition.x <= -Limit)
    {
        FVector Normal = (outPosition.x >= Limit) ? FVector(-1, 0, 0) : FVector(1, 0, 0);

        float dotProduct = Direction.x * Normal.x + Direction.y * Normal.y;
        Direction.x = Direction.x - 2.0f * dotProduct * Normal.x;
        Direction.y = Direction.y - 2.0f * dotProduct * Normal.y;

        outPosition.x = (outPosition.x >= Limit) ? Limit : -Limit;
        bReflected = true;
    }

    if (outPosition.y >= Limit || outPosition.y <= -Limit)
    {
        FVector Normal = (outPosition.y >= Limit) ? FVector(0, -1, 0) : FVector(0, 1, 0);
        float dotProduct = Direction.x * Normal.x + Direction.y * Normal.y;
        Direction.x = Direction.x - 2.0f * dotProduct * Normal.x;
        Direction.y = Direction.y - 2.0f * dotProduct * Normal.y;
        bReflected = true;
    }

    if (bReflected) {
        Direction.x += Random::Range(-0.1f, 0.1f);
        Direction.y += Random::Range(-0.1f, 0.1f);
        Direction.Normalize();
    }

    outRotation = 0.0f;
    return false;
}

void ItemLinearMovement::Reset()
{

}
