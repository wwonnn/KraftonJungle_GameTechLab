#include "InGameScene.h"
#include "SceneManager.h"

InGameScene::InGameScene(FGameContext* gameContext)
    :UScene(gameContext) {
    Timer = gameContext->Timer;

    WaveController = new UWaveController();
    WaveController->LoadStageData(1);
}

void InGameScene::Initialize() { }

void InGameScene::Update(float deltaTime) {}

void InGameScene::Render() {
    Renderer->BeginFrame();

    float deltaTime = Timer->GetDeltaTime();
    WaveController->Update(deltaTime);
    for (UGameObject* const& obj : GameObjects)
    {
        obj->Update(deltaTime);
        obj->Render(*Renderer);
    }

    UImGuiManager::Get().beginFrame();
    ImGui::Text("Total Time: %f", Timer->GetTotalTime());
    ImGui::Text("Delta Time: %f", deltaTime);
    UImGuiManager::Get().endFrame();

    Renderer->SwapBuffer();
}

void InGameScene::Release() {
    delete WaveController;
}
