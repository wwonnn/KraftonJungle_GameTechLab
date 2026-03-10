#pragma once
#include "Renderer.h"
#include "WaveController.h"
#include "InputManager.h"
#include "AudioSystem.h"
#include "ImGuiManager.h"
#include "Timer.h"

struct FGameContext {
    class URenderer* Renderer;
    class UAudioSystem* AudioSystem;
    // class SceneManager* Scene;

    FGameContext(URenderer* r, UAudioSystem* a)
        : Renderer(r), AudioSystem(a)
    {}
};

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

class InitScene :UScene {
public:
    InitScene(FGameContext* gameContext) :UScene(gameContext) {}
    void Update(float deltaTime) override {

    }

};

class InGameScene : public UScene {
private:
    UWaveController* WaveController = nullptr;

public:
    InGameScene(FGameContext* gameContext) :UScene(gameContext){
        WaveController = new UWaveController();
        WaveController->LoadStageData(1);
    }

    void Initialize() override{

    }

    void Update(float deltaTime) override {
        UInputManager::Get().Update();

        GameContext->Renderer->BeginFrame();

        if (UInputManager::Get().GetKeyDown(VK_SPACE))
        {
            OutputDebugString(L"Space key pressed!\n");
            GameContext->AudioSystem->Play("shoot");
        }

        // 오브젝트 Update 및 Render (DeltaTime 사용)
        WaveController->Update(deltaTime);
        for (UGameObject* const& obj : UGameObject::GameObjectList)
        {
            obj->Update(deltaTime);
            obj->Render(*(GameContext->Renderer));
        }

        UImGuiManager::Get().beginFrame();
        // ImGui::Text("Total Time: %f", Timer->GetTotalTime());
        ImGui::Text("Delta Time: %f", deltaTime);
        UImGuiManager::Get().endFrame();

        Render();
    }

    void Render() override {
        GameContext->Renderer->SwapBuffer();
    }

    void Release() override {
        delete WaveController;
    }
};
