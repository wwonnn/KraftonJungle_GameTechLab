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
};

