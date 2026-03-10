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
public:
    virtual ~UScene() {}
    UScene(FGameContext* gameContext) {
        Renderer = gameContext->Renderer;
    }
    virtual void Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void Release() = 0;

    virtual void OnEnter() {}
    virtual void OnExit() {}

    UGameObject* CreateGameObject(UGameObject* obj) {
        GameObjects.push_back(obj);
        return obj;
    }
};
