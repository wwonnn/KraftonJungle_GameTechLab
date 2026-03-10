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
    void UpdateMainMenu();
    void RenderMainMenu();

    void UpdateCredits();
    void RenderCredits();

    void UpdateSettings();
    void RenderSettings();

private:
    std::string State = "MainMenu";

    int OptionIndex = 0;
    int MaxOptions = 3;

    int SettingOptionIndex = 0;
    int MaxsettingOptions = 1;
};

