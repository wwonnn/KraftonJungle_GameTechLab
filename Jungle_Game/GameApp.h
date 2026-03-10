#pragma once
#include "Renderer.h"
#include "AudioSystem.h"
#include "Timer.h"
#include "WaveController.h"
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
    void CreateGameObjects();

    void LoadGameScene();

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    HWND hWnd = nullptr;
    URenderer* Renderer = nullptr;
    UAudioSystem* AudioSystem = nullptr;
    UTimer* Timer = nullptr;
    UWaveController* WaveController = nullptr;

    UScene* introScene = nullptr;
    UScene* ingameScene = nullptr;

    UScene* currScene = nullptr;
};

