#include "SceneManager.h"
#include "InitScene.h"
#include "InGameScene.h"

void SceneManager::ChangeScene(SceneType type) {
    if (currentScene) {
        currentScene->Release();
        delete currentScene;
    }

    switch (type) {
    case SceneType::Init:
        currentScene = new InitScene(GameContext);
        break;
    case SceneType::InGame:
        currentScene = new InGameScene(GameContext);
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
