#include "Scene.h"
#include "Player.h"

class InGameScene : public UScene {
private:
    UWaveController* WaveController = nullptr;

    UPlayer* Player = nullptr;

public:
    InGameScene(FGameContext* gameContext) :UScene(gameContext) {
        WaveController = new UWaveController();
        WaveController->LoadStageData(1);
    }

    ~InGameScene() {
        Release();
    }

    void Initialize() override;

    void Update(float deltaTime) override;

    void Render() override;

    void Release() override {
        delete WaveController;
    }
};
