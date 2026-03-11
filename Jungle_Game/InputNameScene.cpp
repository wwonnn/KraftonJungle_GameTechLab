#include "InputNameScene.h"
#include "SceneManager.h"
#include "GlobalSettings.h"
#include "ScoreManager.h"

void InputNameScene::Initialize() {
    UAudioSystem::Get().Play("win");
}

void InputNameScene::Update(float deltaTime) {
    ProcessKeyInput();
}

void InputNameScene::Render() {
    Renderer->BeginFrame();
    Renderer->DrawString(L"Your Score: " + std::to_wstring(ScoreManager::Get().GetScore()), 400.0f, 250.0f, FVector(1.0f, 1.0f, 1.0f));
    Renderer->DrawString(L"Enter Your Name: " + std::wstring(PlayerName.begin(), PlayerName.end()), 400.0f, 300.0f, FVector(1.0f, 1.0f, 1.0f));
    Renderer->SwapBuffer();
}

void InputNameScene::Release() {
    // Release resources, clean up, etc.
}

void InputNameScene::ProcessKeyInput() {
    for (char key = 'A'; key <= 'Z'; ++key) {
        if (UInputManager::Get().GetKeyDown(key) &&
            PlayerName.length() < MAX_NAME_LENGTH)
        {
            if (UInputManager::Get().GetKey(VK_SHIFT))
            {
                PlayerName += key; // Uppercase
            }
            else
            {
                PlayerName += std::tolower(key); // Lowercase
            }
        }
    }

    if (UInputManager::Get().GetKeyDown(VK_BACK) && !PlayerName.empty()) {
        PlayerName.pop_back();
    }

    if (UInputManager::Get().GetKeyDown(VK_RETURN) && !PlayerName.empty()) {
        SaveScore();
        SceneManager::Get().ChangeScene(SceneType::Scoreboard);
    }
}

void InputNameScene::SaveScore() {
    GlobalSettings::Get().AddScore(PlayerName, ScoreManager::Get().GetScore());
}
