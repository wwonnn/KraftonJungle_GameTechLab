#include "WaveParser.h"
#include "nlohmann/json.hpp"
#include <fstream>

FWaveInfo WaveParser::ParseWaveFile(const std::string& filePath)
{
    FWaveInfo waveInfo;

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        return waveInfo;
    }

    nlohmann::json j;
    file >> j;

    waveInfo.WaveType = j.value("wave_type", "normal");
    waveInfo.EnemyCount = j.value("enemy_count", 0);
    waveInfo.SpawnInterval = j.value("spawn_interval", 0.0f);
    for (auto& pos : j.value("spawn_positions", nlohmann::json::array()))
    {
        waveInfo.SpawnPositions.push_back({
            pos.value("x", 0.0f),
            pos.value("y", 0.0f) });
    }
    for (auto& rot : j.value("spawn_rotations", nlohmann::json::array()))
    {
        waveInfo.SpawnRotations.push_back({
            rot.value("x", 0.0f),
            rot.value("y", 0.0f),
            rot.value("z", 0.0f) });
    }
    for (auto& scale : j.value("spawn_scales", nlohmann::json::array()))
    {
        waveInfo.SpawnScales.push_back({
            scale.value("x", 1.0f),
            scale.value("y", 1.0f),
            scale.value("z", 0.0f) });
    }
    for (auto& strategy : j.value("move_strategies", nlohmann::json::array()))
    {
        waveInfo.MoveStrategies.push_back(strategy);
    }

    return waveInfo;
}
