#include "InGameScene.h"
#include "SceneManager.h"
#include "CollisionSystem.h"

InGameScene::InGameScene(FGameContext* gameContext)
    :UScene(gameContext) {
    Timer = gameContext->Timer;

    WaveController = new UWaveController();
    WaveController->LoadStageData(1);
}

void InGameScene::Initialize()
{
    UGameObject* playerObj = CreateGameObject(new UPlayer(
        FTransform(FVector(-0.2f, -0.8f, 0.0f),
            FVector(0.0f, 0.0f, 0.0f),
            FVector(0.1f, 0.1f, 1.0f))));
    Player = dynamic_cast<UPlayer*>(playerObj);
}

void InGameScene::Update(float deltaTime) {
    WaveController->Update(deltaTime, Player);
    for (UGameObject* const& obj : GameObjects)
    {
        obj->TickDestroyTimer(deltaTime);
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
    UImGuiManager::Get().DrawMyText("Colliders: %d", UCollisionSystem::Get().GetColliders().size());
    UImGuiManager::Get().endFrame();
    UImGuiManager::Get().Update();

    Renderer->SwapBuffer();

    // After all loop logic is done, check it should change the scene or not
    if (Player->IsDead())
    {
        SceneManager::Get().ChangeScene(SceneType::Outro);
    }
}

void InGameScene::Release() {
    UScene::Release();
    delete WaveController;
}
