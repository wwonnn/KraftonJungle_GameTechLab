#include "InfiniteModeLogic.h"
#include "SceneManager.h"
#include <windows.h>

InfiniteModeLogic::InfiniteModeLogic()
    : gen(rd())
{
}

void InfiniteModeLogic::Initialize(UPlayer* player)
{
    ScanWaveFiles(0);
    ScanWaveFiles(1);
    ScanWaveFiles(2);

    Player = player;
}

void InfiniteModeLogic::Update(float deltaTime)
{
    if (SpawnCount >= CurrentStageInfo.enemyCount)
    {
        if (!SpawnFinished)
        {
            SpawnTimer += deltaTime;
            if (SpawnTimer >= 2.0f)
            {
                SpawnFinished = true;
                SpawnTimer = 0.0f;
                StartMove();
            }
        }
        return;
    }

    SpawnTimer += deltaTime;
    if (SpawnTimer >= CurrentStageInfo.spawnInterval)
    {
        auto& pos = CurrentStageInfo.positions[SpawnCount % CurrentStageInfo.positions.size()];
        auto& rot = CurrentStageInfo.rotations[SpawnCount % CurrentStageInfo.rotations.size()];
        auto& scale = CurrentStageInfo.scales[SpawnCount % CurrentStageInfo.scales.size()];

        FTransform transform;
        transform.Location = FVector(pos.x, pos.y, 0);
        transform.Rotation = FVector(rot.x, rot.y, 0);
        transform.Scale = FVector(scale.x, scale.y, 1);
        SpawnEnemy(transform, Player);

        SpawnCount++;
        SpawnTimer = 0.0f;
    }
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
        CurrentStageInfo.rotations.push_back({ rot["x"], rot["y"] });
    }

    CurrentStageInfo.scales.clear();
    for (auto& scale : j["spawn_scales"])
    {
        CurrentStageInfo.scales.push_back({ scale["x"], scale["y"] });
    }

    LeftEnemyCount += CurrentStageInfo.enemyCount;
}

void InfiniteModeLogic::SpawnEnemy(FTransform transform, UPlayer* player)
{
    UGameObject* enemy = SceneManager::Get().GetcurrentScene()->CreateGameObject(new UEnemyObject(transform));
    UEnemyObject* enemyObj = dynamic_cast<UEnemyObject*>(enemy);
    enemyObj->SetPlayer(player);

    auto seq = std::make_unique<MovementSequence>();
    seq->Add(std::make_unique<LinearMovement>(FVector(1.0f, 0.0f), 0.2f), 10.0f);
    seq->Add(std::make_unique<FollowPlayerMovement>(FVector(1.0f, 0.0f), 0.5f), 0.0f);

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
    SpawnCount = 0;
    SpawnTimer = 0.0f;
    SpawnFinished = false;
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
