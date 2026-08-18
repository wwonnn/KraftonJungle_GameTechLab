#pragma once

#include <Windows.h>

#include "Renderer/D3D11/D3D11Common.h"
#include "Renderer/D3D11/D3D11RHI.h"
#include "Renderer/D3D11/GeneralRenderer.h"
#include "Renderer/EditorRenderData.h"
#include "Renderer/SceneRenderData.h"
#include "Renderer/Types/PickResult.h"

class ENGINE_API FRendererModule
{
  public:
    FRendererModule();
    ~FRendererModule();
    
    bool StartupModule(HWND hWnd);
    void ShutdownModule();

    void BeginFrame();
    void EndFrame();

    void SetViewport(const D3D11_VIEWPORT& InViewport);
    void OnWindowResized(int32 InWidth, int32 InHeight);

    void Render(const FEditorRenderData& InEditorRenderData,
                const FSceneRenderData&  InSceneRenderData);
    void Render(const FSceneRenderData& InSceneRenderData);

    bool Pick(const FEditorRenderData& InEditorRenderData, 
              const FSceneRenderData&  InSceneRenderData,
              int32 MouseX, int32 MouseY,
              FPickResult& OutResult);

    FD3D11RHI& GetRHI() { return GeneralRenderer->GetRHI(); }

    FGeneralRenderer* GetGeneralRenderer() { return GeneralRenderer; }

    void SetVSyncEnabled(bool bEnabled);
    bool IsVSyncEnabled() const;

  private:
    FGeneralRenderer*        GeneralRenderer   = nullptr;
    
    TComPtr<ID3D11Debug> DebugDevice;

    void RenderWorldPass(const FEditorRenderData& InEditorRenderData,
                         const FSceneRenderData&  InSceneRenderData);
    void RenderOverlayPass(const FEditorRenderData& InEditorRenderData,
                           const FSceneRenderData&  InSceneRenderData);
};
