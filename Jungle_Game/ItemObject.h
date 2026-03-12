#pragma once
#include <memory>

#include "GameObject.h"
#include "ItemDatabase.h"
#include "MovementStrategies.h"

class UItemObject : public UGameObject
{
public:
    UItemObject();
    UItemObject(FTransform transform);
    UItemObject(FTransform transform, EItemType itemtype);
    ~UItemObject() override;

    EItemType GetID() const { return Itemtype; }

public:
    void Update(float DeltaTime) override;
    void Render(URenderer& renderer) override;

    void OnCollisionEnter(UCircleCollider* other);

private:
    void Initialize();
    std::unique_ptr<IMovementStrategy> CreateStrategy(const FItemTemplate& data);

private:
    FVector Velocity;
    FVector Scale;
    EItemType Itemtype;
    std::unique_ptr<IMovementStrategy> MovementStrategy;
};
