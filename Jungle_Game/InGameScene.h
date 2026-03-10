#include "Scene.h"
#include "Player.h"

class InGameScene : public UScene {
private:
    UWaveController* WaveController = nullptr;
    UTimer* Timer = nullptr;

    UPlayer* Player = nullptr;

    std::vector<UGameObject*> lifeObjects;

public:
    int EnemyKillCount = 0;
    bool bAllStageCleared = false;

public:
    InGameScene(FGameContext* gameContext);
    ~InGameScene() override {}

    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void Release() override;

    void OnEnemyDied();
    void OnAllStageCleared();
};
