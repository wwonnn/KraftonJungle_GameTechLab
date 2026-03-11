#include "InifiniteModeScene.h"
#include "LifeObject.h"
#include "EventSystem.h"
#include "ScoreManager.h"
#include "SceneManager.h"

InfiniteModeScene::InfiniteModeScene(FGameContext* gameContext)
    : UScene(gameContext) {
    Timer = gameContext->Timer;
    Logic = new InfiniteModeLogic();
}

InfiniteModeScene::~InfiniteModeScene() {
    delete Logic;
}

void InfiniteModeScene::Initialize() {
    UGameObject* bgObj = CreateGameObject(new UGameBackground(
        FTransform(FVector(), FVector(), FVector(2.0f, 2.0f, 1.0f))));
    Background = dynamic_cast<UGameBackground*>(bgObj);

    UGameObject* playerObj = CreateGameObject(new UPlayer(
        FTransform(FVector(-0.2f, -0.8f, 0.0f),
            FVector(0.0f, 0.0f, 0.0f),
            FVector(0.1f, 0.1f, 1.0f))));
    Player = dynamic_cast<UPlayer*>(playerObj);

    EventSystem::Get().Subscribe("EnemyDied", std::bind(&InfiniteModeScene::OnEnemyDied, this));

    for (int i = 0; i < 2; ++i)
    {
        float xPos = -0.9f + (i * 0.1f);
        UGameObject* life = CreateGameObject(new LifeObject(
            FTransform(FVector(xPos, -0.9f, 0.0f), FVector(), FVector(0.1f, 0.1f, 1.0f))));

        LifeObjects.push_back(life);
    }

    Player->OnHPChanged = std::bind(&InfiniteModeScene::ChangedHP, this, std::placeholders::_1);

    Logic->Initialize(Player);

    ScoreManager::Get().Initialize();

    UAudioSystem::Get().PlayBGM("bgm");

    Logic->LoadRandomWave();

    CurrWaveText = L"Wave " + std::to_wstring(Logic->GetWaveCount() + 1);
}

void InfiniteModeScene::Update(float deltaTime) {
    Logic->Update(deltaTime);

    for (UGameObject* const& obj : GameObjects)
    {
        obj->TickDestroyTimer(deltaTime);
        obj->Update(deltaTime);
    }
}

void InfiniteModeScene::Render() {
    Renderer->BeginFrame();

    for (UGameObject* const& obj : GameObjects)
    {
        obj->Render(*Renderer);
    }

    if (bWaveClearText)
    {
        Renderer->DrawString(L"Wave Clear!", 400.0f, 300.0f, FVector(1.0f, 1.0f, 0.0f));
    }
    Renderer->DrawString(std::to_wstring(ScoreManager::Get().GetScore()), 50.0f, 50.0f, FVector(1.0f, 1.0f, 1.0f), ETextAlign::Left);
    Renderer->DrawString(CurrWaveText, 750.0f, 50.0f, FVector(1.0f, 1.0f, 1.0f), ETextAlign::Right);

    // Renderer->SwapBuffer();

    // After all loop logic is done, check it should change the scene or not
    if (Player->IsDead())
    {
        SceneManager::Get().ChangeScene(SceneType::InputName);
    }
}

void InfiniteModeScene::Release() {
    UAudioSystem::Get().StopBGM();

    Timer->ClearDelayedActions();
    EventSystem::Get().UnSubscribe("EnemyDied");

    UScene::Release();
}

void InfiniteModeScene::OnEnemyDied()
{
    int leftEnemyCount = Logic->GetLeftEnemyCount();
    Logic->SetLeftEnemyCount(leftEnemyCount - 1);

    if (Logic->GetLeftEnemyCount() <= 0)
    {
        UAudioSystem::Get().Play("clear");

        Background->SetScrollSpeed(1.0f);
        bWaveClearText = true;

        Timer->ExecuteAfter(1.0f, [this]() {
            Background->SetScrollSpeed(0.3f);
            bWaveClearText = false;
            Logic->GoNextWave();

            CurrWaveText = L"Wave " + std::to_wstring(Logic->GetWaveCount() + 1);
        });
    }
}

void InfiniteModeScene::ChangedHP(int newHP)
{
    if (newHP < 0) {
        Player->SetDead(true);
        return;
    }

    while (LifeObjects.size() > newHP)
    {
        if (!LifeObjects.empty())
        {
            LifeObjects.back()->SetPendingDestroy(true);
            LifeObjects.pop_back();
        }
    }

    while (LifeObjects.size() < newHP)
    {
        float xPos = -0.9f + (LifeObjects.size() * 0.1f);
        UGameObject* life = CreateGameObject(new LifeObject(
            FTransform(FVector(xPos, -0.9f, 0.0f), FVector(), FVector(0.1f, 0.1f, 1.0f))));

        LifeObjects.push_back(life);
    }
}
