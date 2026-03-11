#pragma once
#include "GameObject.h"
#include "ItemDatabase.h"

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
    bool CheckCollision(UGameObject* other) override { return true; }
    void ApplyImpulse(const FConstantBuffer& v) override {}

    void OnCollisionEnter(UCircleCollider* other);

private:
    void Initialize();

private:
    FVector Velocity;
    FVector Scale;
    EItemType Itemtype;
};
