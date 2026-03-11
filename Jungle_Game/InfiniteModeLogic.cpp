#include "InfiniteModeLogic.h"
#include "SceneManager.h"
#include "Random.h"
#include <windows.h>

InfiniteModeLogic::InfiniteModeLogic()
    : gen(rd())
{
}

void InfiniteModeLogic::Initialize(UPlayer* player, UTimer* timer)
{
    ScanWaveFiles(0);
    ScanWaveFiles(1);
    ScanWaveFiles(2);

    Player = player;
    Timer = timer;
}

void InfiniteModeLogic::Update(float deltaTime)
{
}

void InfiniteModeLogic::LoadRandomWave()
{
    int difficulty = GetDifficulty();
    std::uniform_int_distribution<size_t> dist(0, WaveFiles[difficulty].size() - 1);
    size_t randomIndex = dist(gen);

    std::string waveFile = WaveFiles[difficulty][randomIndex];

    std::ifstream file(waveFile);
    if (!file.is_open())
    {
        return;
    }

    nlohmann::json j;
    file >> j;

    CurrentStageInfo.enemyCount = j["enemy_count"];
    CurrentStageInfo.spawnInterval = j["spawn_interval"];

    CurrentStageInfo.positions.clear();
    for (auto& pos : j["spawn_positions"])
    {
        CurrentStageInfo.positions.push_back({ pos["x"], pos["y"] });
    }

    CurrentStageInfo.rotations.clear();
    for (auto& rot : j["spawn_rotations"])
    {
        CurrentStageInfo.rotations.push_back({ rot["x"], rot["y"], rot["z"]});
    }

    CurrentStageInfo.scales.clear();
    for (auto& scale : j["spawn_scales"])
    {
        CurrentStageInfo.scales.push_back({ scale["x"], scale["y"] });
    }

    CurrentStageInfo.strategies.clear();
    for (auto& strat : j["move_strategies"])
    {
        CurrentStageInfo.strategies.push_back(strat.get<std::string>());
    }

    LeftEnemyCount += CurrentStageInfo.enemyCount;

    SpawnSequentially();
}

void InfiniteModeLogic::SpawnSequentially()
{
    for (int i = 0; i < CurrentStageInfo.enemyCount; ++i)
    {
        Timer->ExecuteAfter(i * CurrentStageInfo.spawnInterval, [this, i]() {
             auto& pos = CurrentStageInfo.positions[i % CurrentStageInfo.positions.size()];
             auto& rot = CurrentStageInfo.rotations[i % CurrentStageInfo.rotations.size()];
             auto& scale = CurrentStageInfo.scales[i % CurrentStageInfo.scales.size()];
             FTransform transform;
             transform.Location = FVector(pos.x, pos.y, 0);
             transform.Rotation = FVector(rot.x, rot.y, rot.z);
             transform.Scale = FVector(scale.x, scale.y, 1);

             std::string strategy = CurrentStageInfo.strategies[i % CurrentStageInfo.strategies.size()];

             SpawnEnemy(transform, Player, strategy);
            });
    }

    Timer->ExecuteAfter(CurrentStageInfo.enemyCount * CurrentStageInfo.spawnInterval + 1.0f, [this]() {
        StartMove();
        });
}

void InfiniteModeLogic::SpawnEnemy(FTransform transform, UPlayer* player, const std::string& strategy)
{
    UGameObject* enemy = SceneManager::Get().GetcurrentScene()->CreateGameObject(new UEnemyObject(transform));
    UEnemyObject* enemyObj = dynamic_cast<UEnemyObject*>(enemy);
    enemyObj->Initialize();
    enemyObj->SetPlayer(player);

    auto seq = std::make_unique<MovementSequence>(true);

    if (strategy == "aligned")
    {
        seq->Add(std::make_unique<LinearMovement>(FVector(1.0f, 0.0f), 0.2f), 10.0f);
        seq->Add(std::make_unique<FollowPlayerMovement>(FVector(1.0f, 0.0f), 0.5f), 0.0f);
        enemyObj->SetShootable(true);
    }
    else if (strategy == "zigzag")
    {
        seq->Add(std::make_unique<ZigZagMovement>(FVector(0.0f, -1.0f),
            Random::Range(0.4f, 0.6f), Random::Range(0.3f, 0.5f)), 10.0f);
        enemyObj->SetShootable(true);
    }
    else if (strategy == "circular")
    {
        seq->Add(std::make_unique<CircularMovement>(FVector(0.0f, -1.0f),
            Random::Range(0.4f, 0.7f), Random::Range(0.02f, 0.03f), Random::Range(10.0f, 12.0f)), 10.0f);
        enemyObj->SetShootable(false);
    }

    enemyObj->SetMovementStrategy(std::move(seq));

    SpawnedEnemies.push_back(enemyObj);
}

void InfiniteModeLogic::StartMove()
{
    for (auto& enemy : SpawnedEnemies)
    {
        enemy->TransitionToState(EnemyState::Move);
    }
}

void InfiniteModeLogic::GoNextWave()
{
    WaveCount++;
    SpawnedEnemies.clear();
    LoadRandomWave();
}

void InfiniteModeLogic::ScanWaveFiles(int difficulty)
{
    std::string dirPath = WaveDataFilePath + "/difficulty_" + std::to_string(difficulty);
    std::string filePath = dirPath + "/*.json";

    WaveFiles[difficulty] = std::vector<std::string>();
    std::vector<std::string>* files = &WaveFiles[difficulty];
    files->clear();

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(filePath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            std::string fileName = findData.cFileName;
            std::string fullPath = dirPath + "/" + fileName;
            files->push_back(fullPath);
        }
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);
}

void InfiniteModeLogic::ReadWaveFiles()
{
    for (int i = 0; i < 3; ++i)
    {
        auto& fileStrings = WaveFiles[i];

        for (auto& str : fileStrings)
        {
            std::ifstream file(str);
            if (!file.is_open())
            {
                return;
            }

            nlohmann::json j;
            file >> j;

            StageInfo info;

            info.enemyCount = j["enemy_count"];
            info.spawnInterval = j["spawn_interval"];

            info.positions.clear();
            for (auto& pos : j["spawn_positions"])
            {
                info.positions.push_back({ pos["x"], pos["y"] });
            }

            info.rotations.clear();
            for (auto& rot : j["spawn_rotations"])
            {
                info.rotations.push_back({ rot["x"], rot["y"], rot["z"] });
            }

            info.scales.clear();
            for (auto& scale : j["spawn_scales"])
            {
                info.scales.push_back({ scale["x"], scale["y"] });
            }

            info.strategies.clear();
            for (auto& strat : j["move_strategies"])
            {
                info.strategies.push_back(strat.get<std::string>());
            }

            OutputDebugStringA(("Loaded wave file: " + str + "\n").c_str());
            OutputDebugStringA(("Enemy Count: " + std::to_string(info.enemyCount) + "\n").c_str());
            OutputDebugStringA(("Position Count: " + std::to_string(info.positions.size()) + "\n").c_str());
            OutputDebugStringA(("Rotation Count: " + std::to_string(info.rotations.size()) + "\n").c_str());
            OutputDebugStringA(("Scale Count: " + std::to_string(info.scales.size()) + "\n").c_str());
            OutputDebugStringA(("Strategy Count: " + std::to_string(info.strategies.size()) + "\n").c_str());
        }
    }
}
