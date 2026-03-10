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

    FGameContext(class URenderer* r, class UAudioSystem* a)
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

};

class InGameScene : public UScene {
private:
    UWaveController* WaveController = nullptr;
    UTimer* Timer = nullptr;

public:
    InGameScene(FGameContext* gameContext) :UScene(gameContext){
        Timer = new UTimer();
        WaveController = new UWaveController();
        WaveController->LoadStageData(1);
    }

    void Initialize() override{

    }

    void Update(float deltaTime) override {
        UInputManager::Get().Update();
        Timer->Update();

        GameContext->Renderer->BeginFrame();

        if (UInputManager::Get().GetKeyDown(VK_SPACE))
        {
            OutputDebugString(L"Space key pressed!\n");
            GameContext->AudioSystem->Play("shoot");
        }

        // 오브젝트 Update 및 Render (DeltaTime 사용)
        float DeltaTime = Timer->GetDeltaTime();
        WaveController->Update(DeltaTime);
        for (UGameObject* const& obj : UGameObject::GameObjectList)
        {
            obj->Update(DeltaTime);
            obj->Render(*(GameContext->Renderer));
        }

        UImGuiManager::Get().beginFrame();
        ImGui::Text("Total Time: %f", Timer->GetTotalTime());
        ImGui::Text("Delta Time: %f", Timer->GetDeltaTime());
        UImGuiManager::Get().endFrame();

        GameContext->Renderer->SwapBuffer();
    }

    void Render() override {
        // 배경 그리기, 적들 그리기 호출
    }

    void Release() override {
        delete WaveController;
        delete Timer;
    }
};
