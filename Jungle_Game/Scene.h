#pragma once
#include "Renderer.h"
#include "WaveController.h"
#include "InputManager.h"
#include "AudioSystem.h"
#include "ImGuiManager.h"
#include "Timer.h"
#include "GameContext.h"

class SceneManager;

class UScene {
protected:
    FGameContext* GameContext = nullptr;
public:
    virtual ~UScene() {}
    UScene(FGameContext* gameContext) {
        GameContext = gameContext;
    }
    virtual void Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void Release() = 0;

    virtual void OnEnter() {}
    virtual void OnExit() {}
};
