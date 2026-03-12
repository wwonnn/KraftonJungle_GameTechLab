#include "InputNameScene.h"
#include "SceneManager.h"
#include "GlobalSettings.h"
#include "ScoreManager.h"

InputNameScene::InputNameScene(FGameContext* gameContext)
    : UScene(gameContext) {
    Timer = gameContext->Timer;
}

void InputNameScene::Initialize() {
    UAudioSystem::Get().Play("win");
    bCanInput = false;
    Score = 0;

    Timer->ExecuteAfter(1.0f, [this]() {
        bScoreAnimationStart = true;
        });
}

void InputNameScene::Update(float deltaTime) {
    if (bCanInput)
    {
        ProcessKeyInput();
    }

    if (bScoreAnimationStart)
    {
        AnimationElapsedTime += deltaTime;

        float progress = std::min<float>(AnimationElapsedTime / ScoreAnimDuration, 1.0f);
        float easedProgress = 1.0f - std::powf(1.0f - progress, 3.0f);

        Score = static_cast<int>(ScoreManager::Get().GetScore() * easedProgress);

        if (progress >= 1.0f)
        {
            Score = ScoreManager::Get().GetScore();
            bScoreAnimationStart = false;
            bCanInput = true;
        }
    }
}

void InputNameScene::Render() {
    Renderer->BeginFrame();
    Renderer->DrawString(L"Your Score: " + std::to_wstring(Score), 400.0f, 400.0f, FVector(1.0f, 1.0f, 1.0f));

    if (bCanInput)
    {
        Renderer->DrawString(L"Enter Your Name: " + std::wstring(PlayerName.begin(), PlayerName.end()), 400.0f, 500.0f, FVector(1.0f, 1.0f, 1.0f));
    }
}

void InputNameScene::Release() {
    UScene::Release();
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
