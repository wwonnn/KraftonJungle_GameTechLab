#include "GameApp.h"
#include "InputManager.h"
#include "Timer.h"
#include "ImGuiManager.h"
#include "GameObject.h"
#include "Player.h"
#include "EnemyObject.h"
#include "Projectile.h"
#include "CollisionSystem.h"

LRESULT CALLBACK GameApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

GameApp::GameApp()
{
    Renderer = new URenderer();
    AudioSystem = new UAudioSystem();
    Timer = new UTimer();
    WaveController = new UWaveController();
}

GameApp::~GameApp()
{
    delete WaveController;
    delete Timer;
    delete AudioSystem;
    delete Renderer;
}

bool GameApp::Initialize(HINSTANCE hInstance)
{
    WNDCLASS windowClass = {};
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = L"D3D11WindowClass";

    RegisterClass(&windowClass);

    hWnd = CreateWindow(L"D3D11WindowClass", L"Galaga", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 1024, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return false;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    if (!Renderer->Create(hWnd))
    {
        return false;
    }

    if (!AudioSystem->Create())
    {
        return false;
    }

    UImGuiManager::Get().Create(hWnd, Renderer);

    WaveController->LoadStageData(1);

    LoadDefaultAssets();
    CreateGameObjects();

    return true;
}

void GameApp::Run()
{
    Timer->Setup();

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Game Loop
            UInputManager::Get().Update();
            Timer->Update();
            UCollisionSystem::Get().CheckCollisions();

            Renderer->BeginFrame();

            // 오브젝트 Update 및 Render (DeltaTime 사용)
            float DeltaTime = Timer->GetDeltaTime();
            WaveController->Update(DeltaTime);

            //for (UGameObject* const& obj : UGameObject::GameObjectList)
            //{
            //    obj->Update(DeltaTime);
            //    obj->Render(*Renderer);
            //}

            for (size_t i = 0; i < UGameObject::GameObjectList.size(); ++i)
            {
                UGameObject* obj = UGameObject::GameObjectList[i];
                if (obj)
                {
                    obj->Update(DeltaTime);
                    obj->Render(*Renderer);
                }
            }

            UImGuiManager::Get().beginFrame();
            ImGui::Text("Total Time: %f", Timer->GetTotalTime());
            ImGui::Text("Delta Time: %f", Timer->GetDeltaTime());
            UImGuiManager::Get().endFrame();

            Renderer->DrawString("Hello, Galaga!", 0.2f, 100.0f);
            Renderer->DrawString("Press Space to Shoot!", 0.2f, 150.0f);

            Renderer->SwapBuffer();
        }
    }
}

void GameApp::Finalize()
{
    AudioSystem->Release();
    Renderer->Release();
}

void GameApp::LoadDefaultAssets()
{
    AudioSystem->LoadFromFile("asset/shoot.wav", "shoot");

    Renderer->CreateTexture("player.png", "player");
    Renderer->CreateTexture("enemy.png", "enemy");
    Renderer->CreateTexture("projectile.png", "projectile");
}

void GameApp::CreateGameObjects()
{
    UPlayer* player = new UPlayer(
        FTransform(FVector(-0.2f, -0.7f, 0.0f),
            FVector(0.0f, 0.0f, 0.0f),
            FVector(0.3f, 0.3f, 1.0f)));

    UGameObject* enemy = new UEnemyObject(
        FTransform(FVector(-0.2f, -0.7f, 0.0f),
            FVector(0.0f, 0.0f, 0.0f),
            FVector(0.3f, 0.3f, 1.0f)));

    UGameObject* projectile = new UProjectile(
        FTransform(player->GetTransform().Location,
            FVector(0.0f, 0.0f, 0.0f),
            FVector(0.1f, 0.1f, 1.0f)));

    player->SetAudioSystem(AudioSystem);
}
