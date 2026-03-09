#include "CircleCollider.h"
#include "GameObject.h"
#include "CollisionSystem.h"

UCircleCollider::UCircleCollider(UGameObject* owner)
    : Owner(owner)
{
    UCollisionSystem::Get().AddCollider(this);
}

UCircleCollider::~UCircleCollider()
{
    UCollisionSystem::Get().RemoveCollider(this);
}

FVector UCircleCollider::GetCenter() const
{
    if (Owner)
    {
        return Owner->GetTransform().Location + Center;
    }
    return Center;
}

float UCircleCollider::GetRadius() const
{
    if (Owner)
    {
        return Owner->GetTransform().Scale.x * Radius;
    }
    return Radius;
}
