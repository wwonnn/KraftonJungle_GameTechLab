#pragma once
#include "Math.h"
#include <functional>
#include <unordered_map>

enum class ECollisionEvent
{
    Enter,
    Stay,
    Exit
};

class UGameObject;

class UCircleCollider
{
public:
    UCircleCollider() = default;
    UCircleCollider(UGameObject* owner);
    ~UCircleCollider();

public:
    void AddCollisionCallback(ECollisionEvent type, std::function<void()> callback)
    {
        CollisionCallbacks[type] = callback;
    }
    void RemoveCollisionCallback(ECollisionEvent type)
    {
        CollisionCallbacks.erase(type);
    }
    void InvokeCollisionCallback(ECollisionEvent type)
    {
        auto it = CollisionCallbacks.find(type);
        if (it != CollisionCallbacks.end())
        {
            it->second();
        }
    }

    UGameObject* GetOwner() const { return Owner; }
    void SetOwner(UGameObject* owner) { Owner = owner; }

    FVector GetCenter() const;
    void SetCenter(const FVector& center) { Center = center; }

    float GetRadius() const;
    void SetRadius(float radius) { Radius = radius; }

private:
    UGameObject* Owner = nullptr;

    FVector Center = { 0, 0, 0 };
    float Radius = 0.5f;

    std::unordered_map<ECollisionEvent, std::function<void()>> CollisionCallbacks;
};

