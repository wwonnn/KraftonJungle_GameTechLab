#pragma once

#include "Scene.h"

class ScoreboardScene : public UScene
{
public:
    ScoreboardScene(FGameContext* gameContext) : UScene(gameContext) {}

    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void Release() override;
};

