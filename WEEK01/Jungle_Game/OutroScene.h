#pragma once
#include "Scene.h"

class OutroScene : public UScene
{
public:
    OutroScene(FGameContext* gameContext) : UScene(gameContext) {}

    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void Release() override;
};

