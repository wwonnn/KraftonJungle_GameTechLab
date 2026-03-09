#include <windows.h>

#include "Renderer.h"
#include "AudioSystem.h"
#include "InputManager.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

	AudioSystem* audioSystem = new AudioSystem();
    if (!audioSystem->Create())
    {
        return 0;
	}

	audioSystem->LoadFromFile("asset/shoot.wav", "shoot");

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
			UInputManager::Get().Update();
            renderer->Render();

            if (UInputManager::Get().GetKeyDown(VK_SPACE))
            {
				OutputDebugString(L"Space key pressed!\n");
                audioSystem->Play("shoot");
			}
        }
    }

    audioSystem->Release();
    renderer->Release();

    return 0;
}