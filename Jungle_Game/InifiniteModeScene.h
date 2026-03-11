#pragma once

#include "Scene.h"
#include "Player.h"
#include "GameBackground.h"
#include "InfiniteModeLogic.h"

class InfiniteModeScene : public UScene
{
public:
    InfiniteModeScene(FGameContext* gameContext);
    ~InfiniteModeScene() override;

    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void Release() override;

private:
    void OnEnemyDied();
    void ChangedHP(int newHP);
    void LifeInitialize();

private:
    UTimer* Timer = nullptr;

    UPlayer* Player = nullptr;
    UGameBackground* Background = nullptr;

    std::vector<UGameObject*> LifeObjects;

    InfiniteModeLogic* Logic = nullptr;

    bool bWaveClearText = false;

    std::wstring CurrWaveText;
};

