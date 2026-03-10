#include "Scene.h"

class InGameScene : public UScene {
private:
    UWaveController* WaveController = nullptr;

public:
    InGameScene(FGameContext* gameContext) :UScene(gameContext) {
        WaveController = new UWaveController();
        WaveController->LoadStageData(1);
    }

    ~InGameScene() {
        Release();
    }

    void Initialize() override {

    }

    void Update(float deltaTime) override;

    void Render() override {
        GameContext->Renderer->SwapBuffer();
    }

    void Release() override {
        delete WaveController;
    }
};
