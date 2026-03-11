#pragma once
#include "Renderer.h"
#include "WaveController.h"
#include "InputManager.h"
#include "AudioSystem.h"
#include "ImGuiManager.h"
#include "Timer.h"
#include "GameContext.h"
#include "GameObject.h"

#include <vector>

class SceneManager;

class UScene {
protected:
    URenderer* Renderer = nullptr;

    std::vector<UGameObject*> GameObjects;
    std::vector<UGameObject*> PendingGameObjects;
public:
    virtual ~UScene() {}
    UScene(FGameContext* gameContext) {
        Renderer = gameContext->Renderer;
    }
    virtual void Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void Release()
    {
        for (UGameObject* obj : GameObjects) {
            delete obj;
        }
        for (UGameObject* obj : PendingGameObjects) {
            delete obj;
        }

        GameObjects.clear();
        PendingGameObjects.clear();
    }

    virtual void Refresh();

    virtual void OnEnter() {}
    virtual void OnExit() {}

    UGameObject* CreateGameObject(UGameObject* obj) {
        PendingGameObjects.push_back(obj);
        return obj;
    }
};
