#pragma once
#include "Scene.h"

class InitScene :public UScene {
public:
    InitScene(FGameContext* gameContext) :UScene(gameContext) {}
    void Initialize() override {

    }
    void Update(float deltaTime) override;

    void Render() override {
        GameContext->Renderer->SwapBuffer();
    }

    void Release() override {

    }
};
