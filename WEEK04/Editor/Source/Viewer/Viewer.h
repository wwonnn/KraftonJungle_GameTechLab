#pragma once

#include "Renderer/RendererModule.h"
#include "ApplicationCore/Input/InputSystem.h"
#include "Camera/ViewportCamera.h"
#include "ViewerNavigationController.h"
#include "Engine/Game/StaticMeshActor.h"
#include "Engine/Scene/Scene.h"

class FDynamicRHI;
class FAssetObjectManager;

class FViewer
{
  public:
    FViewer();
    ~FViewer();

    void Create();
    void Release();
    void SetRuntimeServices(FD3D11RHI* InRHI, RHI::FDynamicRHI* InDynamicRHI,
                            FAssetObjectManager* InAssetObjectManager);

    void Tick(float DeltaTime, Engine::ApplicationCore::FInputSystem* InputSystem);
    void OnWindowResized(float Width, float Height);

    // 렌더러 연동용
    FSceneView*             GetSceneView() const;
    const FSceneRenderData& GetSceneRenderData() const;

    void BuildRenderData();
    void DrawPanel(HWND hWnd);
    void SetUpView();
    void SetBestView();
    bool TryLoadObjFile(const FWString& FilePath);

  public:
    std::function<void()> OnRequestExit;

  private:
    FSceneView*      SceneView = nullptr;
    FSceneRenderData SceneRenderData;
    FViewportCamera  ViewportCamera;

    FViewerNavigationController NavigationController;

    AStaticMeshActor* StaticMeshActor = nullptr;

    FD3D11RHI*                 RHI = nullptr;
    RHI::FDynamicRHI*          DynamicRHI = nullptr;
    FAssetObjectManager*      AssetObjectManager = nullptr;
};