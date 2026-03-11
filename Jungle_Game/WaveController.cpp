#include "WaveController.h"
#include "SceneManager.h"
#include "EventSystem.h"
#include <fstream>

bool UWaveController::LoadStageData(int stageNumber) {
    std::ifstream file("asset/data/enemies.json");
    if (!file.is_open()) return false;

    nlohmann::json j;
    file >> j;

    std::string key = "stage_" + std::to_string(stageNumber);
    if (!j.contains(key)) return false;

    auto& data = j[key];
    currentStage.stageType = data["stage_type"];
    currentStage.enemyCount = data["enemy_count"];
    currentStage.spawnInterval = data["spawn_interval"];

    currentStage.positions.clear();
    for (auto& pos : data["spawn_positions"]) {
        currentStage.positions.push_back({ pos["x"], pos["y"] });
    }

    currentStage.rotations.clear();
    for (auto& rot : data["spawn_rotations"]) {
        currentStage.rotations.push_back({ rot["x"], rot["y"], rot["z"]});
    }

    currentStage.scales.clear();
    for (auto& scale : data["spawn_scales"]) {
        currentStage.scales.push_back({ scale["x"], scale["y"] });
    }

    spawnedCount = 0;
    spawnTimer = 0.0f;
    spawnFinished = false;
    return true;
}

void UWaveController::Update(float deltaTime, UPlayer * player) {
    if (spawnedCount >= currentStage.enemyCount)
    {
        if(!spawnFinished)
        {
            spawnTimer += deltaTime;
            if (spawnTimer >= 2.0f) {
                spawnFinished = true;
                StartMove();
            }
        }
        return;
    }

    spawnTimer += deltaTime;
    if (spawnTimer >= currentStage.spawnInterval) {
        auto& pos = currentStage.positions[spawnedCount % currentStage.positions.size()];
        auto& rot = currentStage.rotations[spawnedCount % currentStage.rotations.size()];
        auto& scale = currentStage.scales[spawnedCount % currentStage.scales.size()];

        FTransform transform;
        transform.Location = FVector(pos.x, pos.y, 0.0f);
        transform.Rotation = FVector(rot.x, rot.y, rot.z);
        transform.Scale = FVector(scale.x, scale.y, 1.0f);
        SpawnEnemy(transform, player);

        spawnTimer = 0.0f;
        spawnedCount++;
    }
}

void UWaveController::SpawnEnemy(FTransform transform, UPlayer * player) {
    if (currentStage.stageType == "Normal") {
        UGameObject* enemy = SceneManager::Get().GetcurrentScene()->CreateGameObject(new UEnemyObject(transform));
        UEnemyObject* enemyObj = dynamic_cast<UEnemyObject*>(enemy);
        enemyObj->Initialize();
        enemyObj->SetPlayer(player);

        auto seq = std::make_unique<MovementSequence>();
        seq->Add(std::make_unique<LinearMovement>(FVector(1.0f, 0.0f), 0.5f), 10.0f);
        seq->Add(std::make_unique<FollowPlayerMovement>(FVector(1.0f, 0.0f), 0.5f), 0.0f);

        enemyObj->SetMovementStrategy(std::move(seq));
        spawnedEnemies.push_back(enemyObj);
    }
    else if (currentStage.stageType == "Boss") {
        UGameObject* boss = SceneManager::Get().GetcurrentScene()->CreateGameObject(new UBossObject(transform));
        UBossObject* bossObj = dynamic_cast<UBossObject*>(boss);
        bossObj->Initialize();
        bossObj->SetPlayer(player);

        auto seq = std::make_unique<MovementSequence>(true);
        seq->Add(std::make_unique<LinearMovement>(FVector(1.0f, 0.0f), 0.8f, 0.5f), 5.0f);
        seq->Add(std::make_unique<DiveToPlayerMovement>(2.0f), 3.0f);
        seq->Add(std::make_unique<DiveToPlayerMovement>(3.0f), 3.0f);

        bossObj->SetMovementStrategy(std::move(seq));
        spawnedEnemies.push_back(bossObj);
    }
}

void UWaveController::StartMove()
{
    for (auto enemy : spawnedEnemies)
    {
        enemy->TransitionToState(EnemyState::Move);
    }
}

void UWaveController::GoNextStage() {
    currentStageNumber++;
    if (currentStageNumber > totalStages)
    {
        EventSystem::Get().Trigger("AllStageCleared");
        return;
    }

    LoadStageData(currentStageNumber);
}
