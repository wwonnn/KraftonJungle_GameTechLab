#include "CollisionSystem.h"
#include "GameApp.h"

void UCollisionSystem::CheckCollisions()
{
    std::set<FCollisionPair> currentCollisions;

    for (size_t i = 0; i < Colliders.size(); ++i)
    {
        for (size_t j = i + 1; j < Colliders.size(); ++j)
        {
            UCircleCollider* colliderA = Colliders[i];
            UCircleCollider* colliderB = Colliders[j];

            if (CheckCollision(colliderA, colliderB))
            {
                FCollisionPair pair(colliderA, colliderB);
                currentCollisions.insert(pair);

                if (PreviousCollisions.find(pair) == PreviousCollisions.end())
                {
                    colliderA->InvokeCollisionCallback(ECollisionEvent::Enter, colliderB);
                    colliderB->InvokeCollisionCallback(ECollisionEvent::Enter, colliderA);
                }
                else
                {
                    colliderA->InvokeCollisionCallback(ECollisionEvent::Stay, colliderB);
                    colliderB->InvokeCollisionCallback(ECollisionEvent::Stay, colliderA);
                }
            }
        }
    }

    for (const auto& pair : PreviousCollisions)
    {
        if (currentCollisions.find(pair) == currentCollisions.end())
        {
            pair.ColliderA->InvokeCollisionCallback(ECollisionEvent::Exit, pair.ColliderB);
            pair.ColliderB->InvokeCollisionCallback(ECollisionEvent::Exit, pair.ColliderA);
        }
    }

    PreviousCollisions = std::move(currentCollisions);
}

bool UCollisionSystem::CheckCollision(UCircleCollider* colliderA, UCircleCollider* colliderB)
{
    if (colliderA->GetContactLayer().find(colliderB->GetLayer()) == colliderA->GetContactLayer().end())
    {
        return false;
    }
    if (colliderB->GetContactLayer().find(colliderA->GetLayer()) == colliderB->GetContactLayer().end())
    {
        return false;
    }

    FVector centerA = colliderA->GetCenter();
    FVector centerB = colliderB->GetCenter();
    float radiusA = colliderA->GetRadius();
    float radiusB = colliderB->GetRadius();
    float distanceSquared = (centerA - centerB).LengthSquared();
    float radiusSum = radiusA + radiusB;
    return distanceSquared <= (radiusSum * radiusSum);
}

bool UCollisionSystem::CheckWallCollision(FVector& location, FVector& scale)
{
    float left = -1.0f;
    float right = 1.0f;
    float top = -1.0f;
    float bottom = 1.0f;

    FVector halfScale = scale / 2.0f;
    if (location.x - halfScale.x < left)
    {
        location.x = left + halfScale.x;
        return true;
    }
    else if (location.x + halfScale.x > right)
    {
        location.x = right - halfScale.x;
        return true;
    }
    else if (location.y - halfScale.y < top)
    {
        location.y = top + halfScale.y;
        return true;
    }
    else if (location.y + halfScale.y > bottom)
    {
        location.y = bottom - halfScale.y;
        return true;
    }

    return false;
}

void UCollisionSystem::ClearCollisionPair(UGameObject* target)
{
    for (auto it = PreviousCollisions.begin(); it != PreviousCollisions.end(); )
    {
        if (it->ColliderA->GetOwner() == target || it->ColliderB->GetOwner() == target)
        {
            it = PreviousCollisions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
