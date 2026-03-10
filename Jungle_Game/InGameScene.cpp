#include "InGameScene.h"
#include "SceneManager.h"
#include "CollisionSystem.h"
#include "EventSystem.h"
#include "GlobalState.h"

InGameScene::InGameScene(FGameContext* gameContext)
    :UScene(gameContext) {
    Timer = gameContext->Timer;

    WaveController = new UWaveController();
}

void InGameScene::Initialize()
{
    UGameObject* playerObj = CreateGameObject(new UPlayer(
        FTransform(FVector(-0.2f, -0.8f, 0.0f),
            FVector(0.0f, 0.0f, 0.0f),
            FVector(0.1f, 0.1f, 1.0f))));
    Player = dynamic_cast<UPlayer*>(playerObj);

    EventSystem::Get().Subscribe("EnemyDied", std::bind(&InGameScene::OnEnemyDied, this));
    EventSystem::Get().Subscribe("AllStageCleared", std::bind(&InGameScene::OnAllStageCleared, this));

    WaveController->GoNextStage();
}

void InGameScene::Update(float deltaTime) {
    WaveController->Update(deltaTime, Player);
    for (UGameObject* const& obj : GameObjects)
    {
        obj->Update(deltaTime);
    }
}

void InGameScene::Render() {
    Renderer->BeginFrame();

    for (UGameObject* const& obj : GameObjects)
    {
        obj->Render(*Renderer);
    }

    UImGuiManager::Get().beginFrame();
    UImGuiManager::Get().DrawMyText("Enemy Kill Count: %d\nCurrent Stage: %d\n", EnemyKillCount, WaveController->GetCurrentStageInfo().enemyCount);
    UImGuiManager::Get().endFrame();
    UImGuiManager::Get().Update();

    Renderer->SwapBuffer();

    // After all loop logic is done, check it should change the scene or not
    if (Player->IsDead() || bAllStageCleared)
    {
        SceneManager::Get().ChangeScene(SceneType::Outro);
    }
}

void InGameScene::Release() {
    EventSystem::Get().UnSubscribe("EnemyDied");

    UScene::Release();
    delete WaveController;
}

void InGameScene::OnEnemyDied() {
    EnemyKillCount++;

    if (EnemyKillCount >= WaveController->GetCurrentStageInfo().enemyCount)
    {
        EnemyKillCount = 0;
        WaveController->GoNextStage();
    }
}

void InGameScene::OnAllStageCleared()
{
    bAllStageCleared = true;
    GlobalState::Get().bAllStageCleared = true;
}
