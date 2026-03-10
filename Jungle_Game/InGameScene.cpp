#include "InGameScene.h"
#include "SceneManager.h"
#include "CollisionSystem.h"
#include "EventSystem.h"
#include "GlobalState.h"
#include "LifeObject.h"

#include <vector>
#include "Projectile.h"
#include "ScoreManager.h"
#include "GameBackground.h"

InGameScene::InGameScene(FGameContext* gameContext)
    :UScene(gameContext) {
    Timer = gameContext->Timer;

    WaveController = new UWaveController();
}

void InGameScene::Initialize()
{
    UGameObject* bgObj = CreateGameObject(new UGameBackground(
        FTransform(FVector(), FVector(), FVector(2.0f, 2.0f, 1.0f))));
    Background = dynamic_cast<UGameBackground*>(bgObj);
    
    UGameObject* playerObj = CreateGameObject(new UPlayer(
        FTransform(FVector(-0.2f, -0.8f, 0.0f),
            FVector(0.0f, 0.0f, 0.0f),
            FVector(0.1f, 0.1f, 1.0f))));
    Player = dynamic_cast<UPlayer*>(playerObj);

    EventSystem::Get().Subscribe("EnemyDied", std::bind(&InGameScene::OnEnemyDied, this));
    EventSystem::Get().Subscribe("AllStageCleared", std::bind(&InGameScene::OnAllStageCleared, this));

    WaveController->GoNextStage();

    for (int i = 0; i < 2; i++) {
        float xPos = -0.9f + (i * 0.1f);
        UGameObject* life = CreateGameObject(new LifeObject(
            FTransform(FVector(xPos, -0.9f, 0.0f), FVector(), FVector(0.1f, 0.1f, 1.0f))));

        lifeObjects.push_back(life);
    }

    Player->OnHPChanged = [&](int newHP) {
        if (newHP > 0) {
            lifeObjects[newHP - 1]->Destroy();
        }
    };

    UAudioSystem::Get().PlayBGM("bgm");
}

void InGameScene::Update(float deltaTime) {
    WaveController->Update(deltaTime, Player);
    for (UGameObject* const& obj : GameObjects)
    {
        obj->TickDestroyTimer(deltaTime);
        obj->Update(deltaTime);
    }

    if (Player->GetHP() <= 0) SceneManager::Get().ChangeScene(SceneType::Outro);
}

void InGameScene::Render() {
    Renderer->BeginFrame();

    for (UGameObject* const& obj : GameObjects)
    {
        obj->Render(*Renderer);
    }

    Renderer->DrawString(std::to_wstring(ScoreManager::Get().GetScore()), 50.0f, 50.0f, FVector(1.0f, 1.0f, 1.0f));

    if (bStageClearText)
    {
        Renderer->DrawString(L"Stage Clear!", 400.0f, 300.0f, FVector(1.0f, 1.0f, 0.0f));
    }

    UImGuiManager::Get().DrawMyText("Enemy Kill Count: %d\nCurrent Stage: %d\n", EnemyKillCount, WaveController->GetCurrentStageInfo().enemyCount);
    UImGuiManager::Get().Update();

    Renderer->SwapBuffer();

    // After all loop logic is done, check it should change the scene or not
    if (Player->IsDead() || bAllStageCleared)
    {
        SceneManager::Get().ChangeScene(SceneType::Outro);
    }
}

void InGameScene::Release() {
    UAudioSystem::Get().StopBGM();

    Timer->ClearDelayedActions();
    EventSystem::Get().UnSubscribe("EnemyDied");

    UScene::Release();
    delete WaveController;
}

void InGameScene::OnEnemyDied() {
    EnemyKillCount++;

    if (EnemyKillCount >= WaveController->GetCurrentStageInfo().enemyCount)
    {
        UAudioSystem::Get().Play("clear");

        EnemyKillCount = 0;
        bStageClearText = true;

        for (UGameObject* obj : GameObjects)
        {
            if (dynamic_cast<UProjectile*>(obj))
            {
                obj->SetPendingDestroy(true);
            }
        }

        Background->SetScrollSpeed(1.0f);

        Timer->ExecuteAfter(2.0f, [this]() {
            bStageClearText = false;
            Background->SetScrollSpeed(0.3f);
            WaveController->GoNextStage();
        });
    }
}

void InGameScene::OnAllStageCleared()
{
    bAllStageCleared = true;
    GlobalState::Get().bAllStageCleared = true;
}
