#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include "WaveController.h"

class InfiniteModeLogic
{
public:
    InfiniteModeLogic();
    ~InfiniteModeLogic() = default;

public:
    void Initialize(UPlayer* player);
    void Update(float deltaTime);

    void LoadRandomWave();
    void SpawnEnemy(FTransform transform, UPlayer* player);
    void StartMove();

    void GoNextWave();

    int GetWaveCount() const { return WaveCount; }
    int GetDifficulty() const
    {
        if (WaveCount < 3)
        {
            return 0; // Easy
        }
        else if (WaveCount < 6)
        {
            return 1; // Medium
        }
        else
        {
            return 2; // Hard
        }
    }

    int GetLeftEnemyCount() const { return LeftEnemyCount; }
    void SetLeftEnemyCount(int count) { LeftEnemyCount = count; }

private:
    void ScanWaveFiles(int difficulty);

private:
    UPlayer* Player = nullptr;

    const std::string WaveDataFilePath = "asset/data/infinite";
    std::unordered_map<int, std::vector<std::string>> WaveFiles;

    StageInfo CurrentStageInfo;
    int WaveCount = 0;

    int SpawnCount = 0;
    float SpawnTimer = 0.0f;
    bool SpawnFinished = false;
    std::vector<UEnemyObject*> SpawnedEnemies;

    int LeftEnemyCount = 0;

    std::random_device rd;
    std::mt19937 gen;
};

