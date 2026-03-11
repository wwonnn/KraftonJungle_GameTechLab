#pragma once
#include "GameObject.h"

class UItemObject : public UGameObject
{
public:
    UItemObject();
    UItemObject(FTransform transform);
    ~UItemObject() override;

public:
    void Update(float DeltaTime) override;
    void Render(URenderer& renderer) override;

    void OnCollisionEnter(UCircleCollider* other);

private:
    void Initialize();

private:
    FVector Velocity;
};
