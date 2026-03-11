#include "InfiniteModeLogic.h"
#include "SceneManager.h"
#include "Random.h"
#include "WaveParser.h"
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
    CurrentWaveInfo = WaveParser::ParseWaveFile(waveFile);

    LeftEnemyCount += CurrentWaveInfo.EnemyCount;

    SpawnSequentially();
}

void InfiniteModeLogic::SpawnSequentially()
{
    for (int i = 0; i < CurrentWaveInfo.EnemyCount; ++i)
    {
        Timer->ExecuteAfter(i * CurrentWaveInfo.SpawnInterval, [this, i]()
            {
                auto& pos = CurrentWaveInfo.SpawnPositions[i % CurrentWaveInfo.SpawnPositions.size()];
                auto& rot = CurrentWaveInfo.SpawnRotations[i % CurrentWaveInfo.SpawnRotations.size()];
                auto& scale = CurrentWaveInfo.SpawnScales[i % CurrentWaveInfo.SpawnScales.size()];

                FTransform transform;
                transform.Location = FVector(pos.x, pos.y, 0.0f);
                transform.Rotation = FVector(rot.x, rot.y, rot.z);
                transform.Scale = FVector(scale.x, scale.y, 1.0f);

                std::string strategy = CurrentWaveInfo.MoveStrategies[i % CurrentWaveInfo.MoveStrategies.size()];

                SpawnEnemy(transform, Player, strategy);
            }
        );
    }

    Timer->ExecuteAfter(CurrentWaveInfo.EnemyCount * CurrentWaveInfo.SpawnInterval + 1.0f, [this]()
        {
            StartMove();
        }
    );
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
