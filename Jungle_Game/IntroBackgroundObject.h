#pragma once

#include "GameObject.h"

class IntroBackgroundObject : public UGameObject
{
public:
    IntroBackgroundObject();
    IntroBackgroundObject(FTransform transform);
    ~IntroBackgroundObject() override;

public:
    void Update(float DeltaTime) override;
    void Render(URenderer& renderer) override;
};

