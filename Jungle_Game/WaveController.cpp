#include "WaveController.h"
#include <fstream>

bool UWaveController::LoadStageData(int stageNumber) {
    std::ifstream file("enemies.json");
    if (!file.is_open()) return false;

    nlohmann::json j;
    file >> j;

    std::string key = "stage_" + std::to_string(stageNumber);
    if (!j.contains(key)) return false;

    auto& data = j[key];
    currentStage.enemyCount = data["enemy_count"];
    currentStage.spawnInterval = data["spawn_interval"];

    currentStage.positions.clear();
    for (auto& pos : data["spawn_positions"]) {
        currentStage.positions.push_back({ pos["x"], pos["y"] });
    }

    spawnedCount = 0;
    spawnTimer = 0.0f;
    return true;
}

void UWaveController::Update(float deltaTime) {
    if (spawnedCount >= currentStage.enemyCount) return;

    spawnTimer += deltaTime;
    if (spawnTimer >= currentStage.spawnInterval) {
        auto& pos = currentStage.positions[spawnedCount % currentStage.positions.size()];
        SpawnEnemy(pos.x, pos.y);

        spawnTimer = 0.0f;
        spawnedCount++;
    }
}

void UWaveController::SpawnEnemy(float x, float y) {
    UGameObject* enemy = new UEnemyObject(
        FTransform(FVector(x, y, 0.0f),
            FVector(0.0f, 0.0f, 0.0f),
            FVector(0.3f, 0.3f, 1.0f)));
}
