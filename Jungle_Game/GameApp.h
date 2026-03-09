#pragma once
#include "Renderer.h"
#include "AudioSystem.h"
#include <windows.h>

class GameApp
{
public:
    GameApp();
    ~GameApp();

public:
    bool Initialize(HINSTANCE hInstance);
    void Run();
    void Finalize();

private:
    void LoadDefaultAssets();

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    HWND hWnd = nullptr;
    URenderer* Renderer = nullptr;
    UAudioSystem* AudioSystem = nullptr;
};

