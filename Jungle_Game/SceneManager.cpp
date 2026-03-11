#include "SceneManager.h"
#include "IntroScene.h"
#include "InGameScene.h"
#include "OutroScene.h"
#include "InifiniteModeScene.h"
#include "ScoreboardScene.h"
#include "InputNameScene.h"

void SceneManager::Initialize(FGameContext* gameContext) {
    GameContext = gameContext;
}

void SceneManager::ChangeScene(SceneType type) {
    if (currentScene) {
        currentScene->Release();
        delete currentScene;
    }

    switch (type) {
    case SceneType::Init:
        currentScene = new IntroScene(GameContext);
        break;
    case SceneType::InGame:
        currentScene = new InGameScene(GameContext);
        break;
    case SceneType::Outro:
        currentScene = new OutroScene(GameContext);
        break;
    case SceneType::Infinite:
        currentScene = new InfiniteModeScene(GameContext);
        break;
    case SceneType::InputName:
        currentScene = new InputNameScene(GameContext);
        break;
    case SceneType::Scoreboard:
        currentScene = new ScoreboardScene(GameContext);
        break;
    }

    currentScene->Initialize();
}

void SceneManager::Update(float deltaTime)
{
    if (currentScene) {
        currentScene->Refresh();
        currentScene->Update(deltaTime);
    }
}

void SceneManager::Render()
{
    if (currentScene) currentScene->Render();
}
