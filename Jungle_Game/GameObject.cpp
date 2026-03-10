#include "GameObject.h"
#include "InputManager.h"
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
        delete Collider;
    }
}

void UGameObject::Destroy()
{
    bIsPendingDestroy = true;
}


