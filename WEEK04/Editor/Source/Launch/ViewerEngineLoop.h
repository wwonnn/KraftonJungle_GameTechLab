#include "Engine/Asset/AssetObjectManager.h"
#pragma once

#include "Core/CoreMinimal.h"
#include "ApplicationCore/GenericPlatform/IApplication.h"
#include "ApplicationCore/Input/InputSystem.h"
#include "Launch/EngineLoop.h"
#include "Renderer/RendererModule.h"

#if defined(_WIN32)
#include "ApplicationCore/Windows/WindowsApplication.h"
#endif

class FViewer;
class FAssetObjectManager;

namespace RHI
{
    class FDynamicRHI;
}

class FViewerEngineLoop : public IEngineLoop
{
  public:
    bool  PreInit(HINSTANCE HInstance, uint32 NCmdShow) override;
    int32 Run() override;
    void  ShutDown() override;

  private:
    void ProcessSystemMessages();
    void Tick() override;
    void InitializeForTime() override;

    static bool HandleViewerMessage(HWND HWnd, UINT Message, WPARAM WParam, LPARAM LParam,
                                    LRESULT& OutResult, void* UserData);
    bool        HandleWindowResize();
    bool        RunFrameOnce();
    bool        RunFrameOnceWithoutResize();
    void        UpdateFrameTiming();
    Engine::ApplicationCore::FWindowsApplication* GetWindowsApplication() const;

  private:
    Engine::ApplicationCore::IApplication* Application = nullptr;
    Engine::ApplicationCore::FInputSystem* InputSystem = nullptr;

    FViewer*             Viewer = nullptr;
    FRendererModule*     Renderer = nullptr;
    FAssetObjectManager* AssetObjectManager = nullptr;

    int32 CachedWindowWidth = 0;
    int32 CachedWindowHeight = 0;
    bool  bIsExit = false;
    bool  bIsRenderingDuringSizeMove = false;
    bool  bIsInSizeMoveLoop = false;
    bool  bSavedVSyncEnabled = false;

    double PrevTime = 0.0;
    float  DeltaTime = 0.0f;
    float  MainLoopFPS = 0.0f;
};
