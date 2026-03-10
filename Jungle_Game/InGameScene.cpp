#include "InGameScene.h"
#include "SceneManager.h"

void InGameScene::Update(float deltaTime) {
    UInputManager::Get().Update();

    GameContext->Renderer->BeginFrame();

    if (UInputManager::Get().GetKeyDown(VK_SPACE))
    {
        OutputDebugString(L"Space key pressed!\n");
        GameContext->AudioSystem->Play("shoot");
    }

    // 오브젝트 Update 및 Render (DeltaTime 사용)
    WaveController->Update(deltaTime);
    for (UGameObject* const& obj : UGameObject::GameObjectList)
    {
        obj->Update(deltaTime);
        obj->Render(*(GameContext->Renderer));
    }

    UImGuiManager::Get().beginFrame();
    // ImGui::Text("Total Time: %f", Timer->GetTotalTime());
    ImGui::Text("Delta Time: %f", deltaTime);
    if (ImGui::Button("click")) {
        // scene change
    }
    UImGuiManager::Get().endFrame();

}
