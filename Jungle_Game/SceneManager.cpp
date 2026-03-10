#include "SceneManager.h"
#include "IntroScene.h"
#include "InGameScene.h"

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
        break;
    }

    currentScene->Initialize();
}

void SceneManager::Update(float deltaTime)
{
    if (currentScene) currentScene->Update(deltaTime);
}

void SceneManager::Render()
{
    if (currentScene) currentScene->Render();
}
