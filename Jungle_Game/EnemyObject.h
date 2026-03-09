#pragma once

#include "GameObject.h"

class UEnemyObject : public UGameObject
{
public:
    UEnemyObject();
    ~UEnemyObject() override;

public:
    void Update(float DeltaTime) override;
    void Render(URenderer& renderer) override;
    bool CheckCollision(UGameObject* other) override { return true; }
    void ApplyImpulse(const FConstantBuffer& v) override {}
};

