#pragma once
#include <string>
#include <vector>
#include "Math.h"

struct FWaveInfo
{
    std::string WaveType = "normal";
    int EnemyCount = 0;
    float SpawnInterval = 0.0f;
    std::vector<FVector2> SpawnPositions;
    std::vector<FVector> SpawnRotations;
    std::vector<FVector> SpawnScales;
    std::vector<std::string> MoveStrategies;
};

class WaveParser
{
public:
    static FWaveInfo ParseWaveFile(const std::string& filePath);
};

