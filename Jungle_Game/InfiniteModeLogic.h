#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include "WaveController.h"
#include "Timer.h"

class InfiniteModeLogic
{
public:
    InfiniteModeLogic();
    ~InfiniteModeLogic() = default;

public:
    void Initialize(UPlayer* player, UTimer* timer);
    void Update(float deltaTime);

    void LoadRandomWave();
    void SpawnSequentially();
    void SpawnEnemy(FTransform transform, UPlayer* player, int index);
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
    UTimer* Timer = nullptr;
    UPlayer* Player = nullptr;

    const std::string WaveDataFilePath = "asset/data/infinite";
    std::unordered_map<int, std::vector<std::string>> WaveFiles;

    StageInfo CurrentStageInfo;
    int WaveCount = 0;

    std::vector<UEnemyObject*> SpawnedEnemies;

    int LeftEnemyCount = 0;

    std::random_device rd;
    std::mt19937 gen;
};

