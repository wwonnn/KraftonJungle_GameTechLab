#include "InitScene.h"
#include "SceneManager.h"

void InitScene::Update(float deltaTime) {
    UInputManager::Get().Update();

    GameContext->Renderer->BeginFrame();
    UImGuiManager::Get().beginFrame();
    // ImGui::Text("Total Time: %f", Timer->GetTotalTime());
    if (ImGui::Button("next Scene")) {
        SceneManager::Get().ChangeScene(SceneType::InGame);
    }
    UImGuiManager::Get().endFrame();
}
