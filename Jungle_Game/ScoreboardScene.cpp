#include "ScoreboardScene.h"
#include "ScoreManager.h"
#include "SceneManager.h"
#include "GlobalSettings.h"

void ScoreboardScene::Initialize() {
    UAudioSystem::Get().Play("win");
}

void ScoreboardScene::Update(float deltaTime) {
    if (UInputManager::Get().GetKeyDown(VK_SPACE))
    {
        // Return to the intro scene
        UAudioSystem::Get().Play("select");
        SceneManager::Get().ChangeScene(SceneType::Init);
    }
}

void ScoreboardScene::Render() {
    Renderer->BeginFrame();

    Renderer->DrawString(L"Scoreboard", 400.0f, 50.0f, FVector(1.0f, 1.0f, 0.0f));

    const auto& scoreboard = GlobalSettings::Get().GetData().Scoreboard;
    for (size_t i = 0; i < scoreboard.size() && i < 10; ++i) {
        const auto& entry = scoreboard[i];
        std::wstring entryText = std::to_wstring(i + 1) + L". " + std::wstring(entry.Username.begin(), entry.Username.end()) + L" - " + std::to_wstring(entry.Score);
        Renderer->DrawString(entryText, 400.0f, 100.0f + i * 30.0f, FVector(1.0f, 1.0f, 1.0f));
    }
}

void ScoreboardScene::Release() {
    // Release resources, clean up, etc.
}
