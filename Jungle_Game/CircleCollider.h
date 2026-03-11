#pragma once
#include "Math.h"
#include <functional>
#include <set>
#include <unordered_map>

enum class ECollisionEvent
{
    Enter,
    Stay,
    Exit
};

enum class ECollisionLayer
{
    Player,
    Enemy,
    Projectile, // TODO: Renaming to PlayerProjectile
    EnemyProjectile,
    Item
};

class UGameObject;

class UCircleCollider
{
public:
    UCircleCollider() = default;
    UCircleCollider(UGameObject* owner);
    ~UCircleCollider();

public:
    void AddCollisionCallback(ECollisionEvent type, std::function<void(UCircleCollider*)> callback)
    {
        CollisionCallbacks[type] = callback;
    }
    void RemoveCollisionCallback(ECollisionEvent type)
    {
        CollisionCallbacks.erase(type);
    }
    void InvokeCollisionCallback(ECollisionEvent type, UCircleCollider* other)
    {
        auto it = CollisionCallbacks.find(type);
        if (it != CollisionCallbacks.end())
        {
            it->second(other);
        }
    }

    UGameObject* GetOwner() const { return Owner; }
    void SetOwner(UGameObject* owner) { Owner = owner; }

    FVector GetCenter() const;
    void SetCenter(const FVector& center) { Center = center; }

    float GetRadius() const;
    void SetRadius(float radius) { Radius = radius; }

    void SetLayer(ECollisionLayer layer) { Layer = layer; }
    ECollisionLayer GetLayer() const { return Layer; }

    void AddContactLayer(ECollisionLayer layer) { ContactLayer.insert(layer); }
    void RemoveContactLayer(ECollisionLayer layer) { ContactLayer.erase(layer); }
    const std::set<ECollisionLayer>& GetContactLayer() const { return ContactLayer; }

private:
    UGameObject* Owner = nullptr;

    FVector Center = { 0, 0, 0 };
    float Radius = 0.25f;

    std::unordered_map<ECollisionEvent, std::function<void(UCircleCollider*)>> CollisionCallbacks;

    ECollisionLayer Layer = ECollisionLayer::Player;
    std::set<ECollisionLayer> ContactLayer;
};

