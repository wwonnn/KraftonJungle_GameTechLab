#pragma once
#include "Scene.h"
#include "GameContext.h"


class SceneManager {
private:
    UScene* currentScene = nullptr;
    FGameContext* GameContext = nullptr;

public:
    static SceneManager& Get()
    {
        static SceneManager instance;
        return instance;
    }

    SceneManager() = default;
    ~SceneManager() = default;

    void Initialize(FGameContext* gameContext);
    void ChangeScene(SceneType type);
    void Update(float deltaTime);
    void Render();

    void Release();

    UScene* GetcurrentScene() const { return currentScene; }
};
