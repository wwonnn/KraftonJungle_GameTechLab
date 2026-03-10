#include "GameApp.h"
#include "InputManager.h"
#include "Timer.h"
#include "ImGuiManager.h"
#include "GameObject.h"
#include "GameObjectManager.h"
#include "Player.h"
#include "EnemyObject.h"
#include "Projectile.h"
#include "CollisionSystem.h"
#include "IntroScene.h"
#include "EventSystem.h"

#include "SceneManager.h"
#include "GameContext.h"
#include "GlobalSettings.h"

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
    Timer = new UTimer();
}

GameApp::~GameApp()
{
    delete Timer;
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

    GlobalSettings::Get().Load();

    if (!Renderer->Create(hWnd))
    {
        return false;
    }

    if (!UAudioSystem::Get().Create())
    {
        return false;
    }

    UImGuiManager::Get().Create(hWnd, Renderer);

    LoadDefaultAssets();

     SceneManager::Get().Initialize(new FGameContext(Renderer, Timer));
     SceneManager::Get().ChangeScene(SceneType::Init);
    //ingameScene = new InGameScene(&gameContext);

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

            SceneManager::Get().Update(Timer->GetDeltaTime());

            SceneManager::Get().Render();
        }
    }
}

void GameApp::Finalize()
{
    UImGuiManager::Get().Release();

    UAudioSystem::Get().Release();
    Renderer->Release();
}

void GameApp::LoadDefaultAssets()
{
    UAudioSystem::Get().LoadFromFile("asset/audio/shoot.mp3", "shoot");
    UAudioSystem::Get().LoadFromFile("asset/audio/select.mp3", "select");
    UAudioSystem::Get().LoadFromFile("asset/audio/choose.mp3", "choose");
    UAudioSystem::Get().LoadFromFile("asset/audio/pop.mp3", "pop");
    UAudioSystem::Get().LoadFromFile("asset/audio/clear.mp3", "clear");
    UAudioSystem::Get().LoadFromFile("asset/audio/win.mp3", "win");
    UAudioSystem::Get().LoadFromFile("asset/audio/gameover.mp3", "gameover");
    UAudioSystem::Get().LoadFromFile("asset/audio/whoosh.mp3", "whoosh");
    UAudioSystem::Get().LoadFromFile("asset/audio/bgm.mp3", "bgm");

    Renderer->CreateTexture("asset/texture/player.png", "player");
    Renderer->CreateTexture("asset/texture/enemy.png", "enemy");
    Renderer->CreateTexture("asset/texture/background.png", "background");
    Renderer->CreateTexture("asset/texture/title.png", "title");
    Renderer->CreateTexture("asset/texture/projectile.png", "projectile");
    Renderer->CreateTexture("asset/texture/game_bg.png", "game_bg");
}
