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
};

class UWaveController {
private:
    StageInfo currentStage;
    float spawnTimer = 0.0f;
    int spawnedCount = 0;

public:
    bool LoadStageData(int stageNumber);
    void Update(float deltaTime, UPlayer* player);
    void SpawnEnemy(float x, float y, UPlayer* player);
};
