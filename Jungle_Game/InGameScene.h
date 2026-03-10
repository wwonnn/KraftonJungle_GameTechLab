#include "Scene.h"
#include "Player.h"

class InGameScene : public UScene {
private:
    UWaveController* WaveController = nullptr;
    UTimer* Timer = nullptr;

    UPlayer* Player = nullptr;

public:
    InGameScene(FGameContext* gameContext);
    ~InGameScene() {
        Release();
    }

    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void Release() override;
};
