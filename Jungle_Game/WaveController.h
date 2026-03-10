#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "nlohmann/json.hpp"

#include "GameObject.h"
#include "EnemyObject.h"

using json = nlohmann::json;

struct SpawnPosition {
    float x, y;
};

struct StageInfo {
    int enemyCount;
    float spawnInterval;
    std::vector<SpawnPosition> positions;
    std::vector<FVector> rotations;
    std::vector<FVector> scales;
};

class UWaveController {
private:
    StageInfo currentStage;
    float spawnTimer = 0.0f;
    int spawnedCount = 0;
    bool spawnFinished = false;
    std::vector<UEnemyObject*> spawnedEnemies;

    int currentStageNumber = 0;
    int totalStages = 2;

public:
    bool LoadStageData(int stageNumber);
    void Update(float deltaTime, UPlayer* player);
    void SpawnEnemy(FTransform transform, UPlayer* player);
    void StartMove();

    void GoNextStage();

    StageInfo GetCurrentStageInfo() const { return currentStage; }
};
