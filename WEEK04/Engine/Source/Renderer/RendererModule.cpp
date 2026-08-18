#include "Renderer/RendererModule.h"

#include "SceneView.h"
#include "Engine/Component/Mesh/LineBatchComponent.h"
#include "Renderer/Types/PickId.h"
#include "Renderer/Types/PickResult.h"

FRendererModule::FRendererModule()
{
}

FRendererModule::~FRendererModule()
{
}

bool FRendererModule::StartupModule(HWND hWnd)
{
    if (hWnd == nullptr)
    {
        return false;
    }
    
    RECT ClientRect = {};
    if (!GetClientRect(hWnd, &ClientRect))
    {
        return false;
    }
    int ViewportWidth = static_cast<int32>(ClientRect.right - ClientRect.left);
    int ViewportHeight = static_cast<int32>(ClientRect.bottom - ClientRect.top);
    
    GeneralRenderer = new FGeneralRenderer(hWnd, ViewportWidth, ViewportHeight);
    
    if (GeneralRenderer->GetDevice() != nullptr)
    {
#if defined(_DEBUG)
        GeneralRenderer->GetDevice()->QueryInterface(__uuidof(ID3D11Debug),
                                        reinterpret_cast<void**>(DebugDevice.GetAddressOf()));
#endif
    }

    return true;
}

void FRendererModule::ShutdownModule()
{
    DebugDevice.Reset();

#if defined(_DEBUG)
    if (DebugDevice != nullptr)
    {
        DebugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        DebugDevice.Reset();
    }
#endif

    if (GeneralRenderer)
    {
        delete GeneralRenderer;
        GeneralRenderer = nullptr;
    }
}

void FRendererModule::BeginFrame()
{
    GeneralRenderer->BeginFrame();
}

void FRendererModule::EndFrame()
{
    GeneralRenderer->EndFrame();
}

void FRendererModule::SetViewport(const D3D11_VIEWPORT& InViewport) 
{ 
    GeneralRenderer->GetRHI().SetViewport(InViewport); 
}

void FRendererModule::OnWindowResized(int32 InWidth, int32 InHeight)
{
    if (GeneralRenderer)
    {
        GeneralRenderer->OnResize(InWidth, InHeight);
    }
}

void FRendererModule::Render(const FEditorRenderData& InEditorRenderData,
                             const FSceneRenderData&  InSceneRenderData)
{
    RenderWorldPass(InEditorRenderData, InSceneRenderData);
    RenderOverlayPass(InEditorRenderData, InSceneRenderData);
}

void FRendererModule::Render(const FSceneRenderData& InSceneRenderData)
{
    RenderWorldPass({}, InSceneRenderData);
}

void FRendererModule::RenderWorldPass(const FEditorRenderData& InEditorRenderData,
                                      const FSceneRenderData&  InSceneRenderData)
{
    if (GeneralRenderer)
    {
        GeneralRenderer->ClearCommandList();

        FRenderCommandQueue CommandQueue;
        for (const auto& el : InSceneRenderData.RenderCommands)
        {
            CommandQueue.AddCommand(el);
        }
        
        if (InSceneRenderData.SceneView)
        {
            CommandQueue.ViewMatrix = InSceneRenderData.SceneView->GetViewMatrix();
            CommandQueue.ProjectionMatrix = InSceneRenderData.SceneView->GetProjectionMatrix();
        }
        
        GeneralRenderer->SubmitCommands(CommandQueue);
        GeneralRenderer->ExecuteCommands();
    }
}

void FRendererModule::RenderOverlayPass(const FEditorRenderData& InEditorRenderData,
                                        const FSceneRenderData&  InSceneRenderData)
{
    // Gizmo etc. are now submitted as RenderCommands from EditorViewportClient
}

bool FRendererModule::Pick(const FEditorRenderData& InEditorRenderData,
                           const FSceneRenderData&  InSceneRenderData,
                           int32 MouseX, int32 MouseY,
                           FPickResult& OutResult)
{
    if (!GeneralRenderer) return false;
    
    // Clear and Submit commands for this specific pick pass
    GeneralRenderer->ClearCommandList();
    
    FRenderCommandQueue CommandQueue;
    for (const auto& Cmd : InSceneRenderData.RenderCommands)
    {
        CommandQueue.AddCommand(Cmd);
    }
    
    if (InSceneRenderData.SceneView)
    {
        CommandQueue.ViewMatrix = InSceneRenderData.SceneView->GetViewMatrix();
        CommandQueue.ProjectionMatrix = InSceneRenderData.SceneView->GetProjectionMatrix();
    }
    
    GeneralRenderer->SubmitCommands(CommandQueue);

    uint32 PickId = 0;
    // MouseX, MouseY는 이미 월드 좌표계(윈도우 전체 상대)이므로 그대로 전달
    if (GeneralRenderer->Pick(MouseX, MouseY, PickId))
        OutResult = PickResult::FromPickId(PickId);
    
    return PickId != 0;
}

void FRendererModule::SetVSyncEnabled(bool bEnabled) 
{ 
    if (GeneralRenderer) GeneralRenderer->SetVSync(bEnabled); 
}

bool FRendererModule::IsVSyncEnabled() const 
{ 
    return GeneralRenderer ? GeneralRenderer->IsVSyncEnabled() : false; 
}
