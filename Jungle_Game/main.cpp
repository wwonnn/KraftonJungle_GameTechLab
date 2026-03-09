#include <windows.h>

#include "Renderer.h"
#include "AudioSystem.h"
#include "InputManager.h"
#include "ImGuiManager.h"
#include "Timer.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

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

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    WNDCLASS windowClass = {};
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = L"D3D11WindowClass";

    RegisterClass(&windowClass);

    HWND hWnd = CreateWindow(L"D3D11WindowClass", L"Galaga", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 1024, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    URenderer* renderer = new URenderer();
    if (!renderer->Create(hWnd))
    {
        return 0;
    }

    UAudioSystem* audioSystem = new UAudioSystem();
    if (!audioSystem->Create())
    {
        return 0;
    }

    audioSystem->LoadFromFile("asset/shoot.wav", "shoot");

    UImGuiManager::Get().Create(hWnd, renderer);

    Timer* timer = new Timer();

    MSG msg = {};
    timer->Setup();
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            UInputManager::Get().Update();
            renderer->Render();

            if (UInputManager::Get().GetKeyDown(VK_SPACE))
            {
                OutputDebugString(L"Space key pressed!\n");
                audioSystem->Play("shoot");
            }

            timer->Update();
            UImGuiManager::Get().beginFrame();
            ImGui::Text("Total Time: %f", timer->GetTotalTime());
            ImGui::Text("Delta Time: %f", timer->GetDeltaTime());
            UImGuiManager::Get().endFrame();

            renderer->SwapBuffer();
        }


    }


    UImGuiManager::Get().Release();
    audioSystem->Release();
    renderer->Release();

    return 0;
}
