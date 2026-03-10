#include "GameApp.h"
#include "InputManager.h"
#include "Timer.h"
#include "ImGuiManager.h"
#include "GameObject.h"
#include "Player.h"
#include "EnemyObject.h"

#include "SceneManager.h"
#include "GameContext.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK GameApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

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

    SceneManager::Get().Initialize(new FGameContext(Renderer, AudioSystem));
    SceneManager::Get().ChangeScene(SceneType::Init);

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
            Timer->Update();
            float DeltaTime = Timer->GetDeltaTime();
            SceneManager::Get().Update(DeltaTime);
            SceneManager::Get().Render();
            // ingameScene->Update(DeltaTime);
            
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
}

void GameApp::CreateGameObjects()
{
    UGameObject* player = new UPlayer(
        FTransform(FVector(-0.2f, -0.7f, 0.0f),
            FVector(0.0f, 0.0f, 0.0f),
            FVector(0.3f, 0.3f, 1.0f)));

}
