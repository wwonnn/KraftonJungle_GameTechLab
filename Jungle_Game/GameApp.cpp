#include "GameApp.h"

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
}

GameApp::~GameApp()
{
    delete Renderer;
    delete AudioSystem;
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

    LoadDefaultAssets();

    return true;
}

void GameApp::Run()
{
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
            Renderer->Render();
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
}
