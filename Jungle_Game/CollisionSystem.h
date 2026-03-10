#pragma once

#include "CircleCollider.h"
#include <vector>
#include <set>
#include <windows.h>

struct FCollisionPair
{
    UCircleCollider* ColliderA;
    UCircleCollider* ColliderB;
    FCollisionPair(UCircleCollider* a, UCircleCollider* b)
        : ColliderA(a), ColliderB(b) {
    }

    bool operator<(const FCollisionPair& other) const
    {
        if (ColliderA != other.ColliderA)
            return ColliderA < other.ColliderA;
        return ColliderB < other.ColliderB;
    }

    bool operator==(const FCollisionPair& other) const
    {
        return ColliderA == other.ColliderA && ColliderB == other.ColliderB;
    };
};

class UCollisionSystem
{
public:
    UCollisionSystem() = default;
    ~UCollisionSystem() = default;

    static UCollisionSystem& Get()
    {
        static UCollisionSystem instance;
        return instance;
    }

public:
    void AddCollider(UCircleCollider* collider) { Colliders.push_back(collider); }
    void RemoveCollider(UCircleCollider* collider)
    {
        auto it = std::find(Colliders.begin(), Colliders.end(), collider);
        if (it != Colliders.end())
        {
            Colliders.erase(it);
        }
    }
    void CheckCollisions();
    bool CheckWallCollision(FVector& location, FVector& scale);

private:
    bool CheckCollision(UCircleCollider* colliderA, UCircleCollider* colliderB);

private:
    std::vector<UCircleCollider*> Colliders;
    std::set<FCollisionPair> PreviousCollisions;
};

