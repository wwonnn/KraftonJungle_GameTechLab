#pragma once

#include "GameObject.h"

class UGameBackground : public UGameObject
{
public:
    UGameBackground();
    UGameBackground(FTransform transform);
    ~UGameBackground() override;

public:
    void Update(float DeltaTime) override;
    void Render(URenderer& renderer) override;

    void SetScrollSpeed(float speed) { ScrollSpeed = speed; }

private:
    float ScrollSpeed = 0.3f;
};

