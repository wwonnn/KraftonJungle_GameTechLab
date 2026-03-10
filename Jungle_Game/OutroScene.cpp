#include "OutroScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "GlobalState.h"
#include "AudioSystem.h"

void OutroScene::Initialize() {
    if (GlobalState::Get().bAllStageCleared)
    {
        UAudioSystem::Get().Play("win");
    }
    else
    {
        UAudioSystem::Get().Play("gameover");
    }
}

void OutroScene::Update(float deltaTime) {
    if (UInputManager::Get().GetKeyDown(VK_SPACE))
    {
        // Return to the intro scene
        SceneManager::Get().ChangeScene(SceneType::Init);
    }
}

void OutroScene::Render() {
    Renderer->BeginFrame();

    if (GlobalState::Get().bAllStageCleared)
    {
        Renderer->DrawString(L"Congratulations! You Cleared All Stages!", 400.0f, 300.0f, FVector(0.0f, 1.0f, 0.0f));
    }
    else
    {
        Renderer->DrawString(L"Game Over!", 400.0f, 300.0f, FVector(1.0f, 0.0f, 0.0f));
    }
    Renderer->DrawString(L"Press Space to Return to Main Menu", 400.0f, 350.0f, FVector(1.0f, 1.0f, 1.0f));

    Renderer->SwapBuffer();
}

void OutroScene::Release() {
    // Release resources, clean up, etc.
}
