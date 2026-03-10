#pragma once
#include "GameObject.h"

class LifeObject : public UGameObject
{
public:
    LifeObject();
    LifeObject(FTransform transform);
    ~LifeObject() override;

public:
    void Update(float DeltaTime) override;
    void Render(URenderer& renderer) override;
    bool CheckCollision(UGameObject* other) override { return false; }
    void ApplyImpulse(const FConstantBuffer& v) override {}
};

