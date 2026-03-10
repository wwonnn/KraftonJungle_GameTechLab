#include "InGameScene.h"
#include "SceneManager.h"
#include "CollisionSystem.h"

void InGameScene::Initialize()
{
    UGameObject* playerObj = CreateGameObject(new UPlayer(
        FTransform(FVector(-0.2f, -0.7f, 0.0f),
            FVector(0.0f, 0.0f, 0.0f),
            FVector(0.3f, 0.3f, 1.0f))));
    Player = dynamic_cast<UPlayer*>(playerObj);
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

    // 오브젝트 Update 및 Render (DeltaTime 사용)

    for (UGameObject* const& obj : GameObjects)
    {
        obj->Render(*Renderer);
    }

    UImGuiManager::Get().beginFrame();
    UImGuiManager::Get().DrawMyText("Colliders: %d", UCollisionSystem::Get().GetColliders().size());
    UImGuiManager::Get().endFrame();
    UImGuiManager::Get().Update();

    Renderer->SwapBuffer();
}
