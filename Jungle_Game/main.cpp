#include <windows.h>

#include "Renderer.h"
#include "AudioSystem.h"
#include "InputManager.h"
#include "ImGuiManager.h"
#include "Timer.h"

// 게임 오브젝트
#include "GameObject.h"
#include "Player.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void InitGame()
{
    // 게임 초기화(초기 오브젝트 생성 등)

    // 플레이어 생성
    UGameObject* Player1 = new UPlayer(
        FTransform(FVector(-0.2f, -0.7f, 0.0f), FVector(0.0f, 0.0f, 0.0f), FVector(0.3f, 0.3f, 0.3f))
    );
    UGameObject* Player2 = new UPlayer(
        FTransform(FVector(0.2f, -0.7f, 0.0f), FVector(0.0f, 0.0f, 0.0f), FVector(0.3f, 0.3f, 0.3f))
    );
}

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
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024, nullptr, nullptr, hInstance, nullptr);

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

    InitGame();

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
            //renderer->Render();

            renderer->BeginFrame();

            if (UInputManager::Get().GetKeyDown(VK_SPACE))
            {
                OutputDebugString(L"Space key pressed!\n");
                audioSystem->Play("shoot");
            }

            // 오브젝트 Update 및 Render (DeltaTime 사용)
            float DeltaTime = timer->GetDeltaTime();
            for (UGameObject* const& obj : UGameObject::GameObjectList)
            {
                obj->Update(DeltaTime);
                obj->Render(*renderer);
            }

            // 프레임 타이밍 업데이트
            timer->Update();
            UImGuiManager::Get().beginFrame();
            ImGui::Text("Total Time: %f", timer->GetTotalTime());
            ImGui::Text("Delta Time: %f", timer->GetDeltaTime());
            UImGuiManager::Get().endFrame();

            renderer->EndFrame();
            renderer->SwapBuffer();
        }
    }


    UImGuiManager::Get().Release();
    audioSystem->Release();
    renderer->Release();

    return 0;
}
