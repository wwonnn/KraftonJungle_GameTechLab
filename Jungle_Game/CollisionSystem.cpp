#include "CollisionSystem.h"

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
                    colliderA->InvokeCollisionCallback(ECollisionEvent::Enter);
                    colliderB->InvokeCollisionCallback(ECollisionEvent::Enter);
                }
                else
                {
                    colliderA->InvokeCollisionCallback(ECollisionEvent::Stay);
                    colliderB->InvokeCollisionCallback(ECollisionEvent::Stay);
                }
            }
        }
    }

    for (const auto& pair : PreviousCollisions)
    {
        if (currentCollisions.find(pair) == currentCollisions.end())
        {
            pair.ColliderA->InvokeCollisionCallback(ECollisionEvent::Exit);
            pair.ColliderB->InvokeCollisionCallback(ECollisionEvent::Exit);
        }
    }

    PreviousCollisions = std::move(currentCollisions);
}

bool UCollisionSystem::CheckCollision(UCircleCollider* colliderA, UCircleCollider* colliderB)
{
    FVector centerA = colliderA->GetCenter();
    FVector centerB = colliderB->GetCenter();
    float radiusA = colliderA->GetRadius();
    float radiusB = colliderB->GetRadius();
    float distanceSquared = (centerA - centerB).LengthSquared();
    float radiusSum = radiusA + radiusB;
    return distanceSquared <= (radiusSum * radiusSum);
}
