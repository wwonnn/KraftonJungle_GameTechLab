#pragma once
#include "Scene.h"

class InputNameScene : public UScene
{
public:
    InputNameScene(FGameContext* gameContext) : UScene(gameContext) {}

    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void Release() override;

private:
    void ProcessKeyInput();
    void SaveScore();

private:
    std::string PlayerName;
    const int MAX_NAME_LENGTH = 10;
};

