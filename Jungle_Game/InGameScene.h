#include "Scene.h"
#include "Player.h"
#include "GameBackground.h"

class InGameScene : public UScene {
private:
    UWaveController* WaveController = nullptr;
    UTimer* Timer = nullptr;

    UPlayer* Player = nullptr;
    UGameBackground* Background = nullptr;

    std::vector<UGameObject*> lifeObjects;

public:
    int EnemyKillCount = 0;
    bool bStageClearText = false;
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

    void ChangedHP(int newHP);
};
