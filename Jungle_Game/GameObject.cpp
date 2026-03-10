#include "GameObject.h"
#include "InputManager.h"
#include "CollisionSystem.h"
#include <cmath>

UGameObject::UGameObject()
{
    Transform = FTransform();
    Collider = new UCircleCollider(this);
}

UGameObject::UGameObject(FTransform transform)
    : Transform(transform)
{
    Collider = new UCircleCollider(this);
}

UGameObject::~UGameObject()
{
    if (Collider) {
        UCollisionSystem::Get().ClearCollisionPair(this);
        delete Collider;
    }
}

void UGameObject::Destroy()
{
    bIsPendingDestroy = true;
}

void UGameObject::Destroy(float delay)
{
    if (delay <= 0.0f)
    {
        Destroy();
    }
    else
    {
        DestroyDelayTimer = delay;
    }
}

// This function should be called every frame to update the destroy timer if it's set.
void UGameObject::TickDestroyTimer(float deltaTime)
{
    if (DestroyDelayTimer < 0.0f) return;

    DestroyDelayTimer -= deltaTime;
    if (DestroyDelayTimer <= 0.0f)
    {
        Destroy();
    }
}


