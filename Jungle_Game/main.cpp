#include <windows.h>

#include "Renderer.h"

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

    // 2. 윈도우 생성
    HWND hWnd = CreateWindowW(L"D3D11WindowClass", L"DirectX 11 Renderer", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 800, 600, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    URenderer* renderer  = new URenderer();
    if (!renderer->Create(hWnd))
    {
        return 0;
    }

    // 4. 메시지 루프 (Game Loop)
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // 메시지가 있으면 처리
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {

        }
    }

    renderer->Release();

    return 0;
}