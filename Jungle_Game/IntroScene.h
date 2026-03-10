#pragma once

#include "Scene.h"

class IntroScene : public UScene
{
public:
    IntroScene(FGameContext* gameContext) : UScene(gameContext) {}

    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void Release() override;

private:
    int OptionIndex = 0;
    int MaxOptions = 2;

    bool IsCredit = false;
};

