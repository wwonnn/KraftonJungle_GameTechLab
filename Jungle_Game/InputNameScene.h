#pragma once
#include "Scene.h"

class InputNameScene : public UScene
{
public:
    InputNameScene(FGameContext* gameContext);

    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void Release() override;

private:
    void ProcessKeyInput();
    void SaveScore();

private:
    UTimer* Timer = nullptr;

    bool bScoreAnimationStart = false;
    bool bCanInput = false;

    float ScoreAnimDuration = 2.0f;
    float AnimationElapsedTime = 0.0f;
    int Score = 0;

    std::string PlayerName;
    const int MAX_NAME_LENGTH = 10;
};

