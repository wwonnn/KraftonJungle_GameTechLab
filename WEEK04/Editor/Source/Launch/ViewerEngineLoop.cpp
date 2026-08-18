#include "ViewerEngineLoop.h"
#include "Viewer/Viewer.h"
#include "Renderer/SceneView.h"
#include "Engine/Asset/AssetObjectManager.h"
#include "RHI/D3D11/D3D11DynamicRHI.h"
#include "Asset/Manager/AssetCacheManager.h"

#include <windows.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "Core/Misc/NameSubsystem.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND HWnd, UINT Message, WPARAM WParam,
                                                             LPARAM LParam);

bool FViewerEngineLoop::PreInit(HINSTANCE HInstance, uint32 NCmdShow)
{
    (void)HInstance;
    (void)NCmdShow;

    Engine::Core::Misc::FNameSubsystem::Init();
    InputSystem = new Engine::ApplicationCore::FInputSystem();

#if defined(_WIN32)
    Application = Engine::ApplicationCore::FWindowsApplication::Create();
    if (Application == nullptr)
    {
        return false;
    }

    Engine::ApplicationCore::FWindowsApplication* WindowsApplication = GetWindowsApplication();
    if (WindowsApplication == nullptr)
    {
        return false;
    }

    if (!WindowsApplication->CreateEditorWindow(L"OBJ Viewer", 1280, 720))
    {
        return false;
    }

    Application->SetInputSystem(InputSystem);
#else
    return false;
#endif

    Viewer = new FViewer();
    Viewer->Create();
    Viewer->OnRequestExit = [this]() { bIsExit = true; };

    Renderer = new FRendererModule();
    if (Renderer == nullptr)
    {
        return false;
    }

    HWND WindowHandle = static_cast<HWND>(Application->GetNativeWindowHandle());
    if (WindowHandle == nullptr)
    {
        return false;
    }

    if (!Renderer->StartupModule(WindowHandle))
    {
        return false;
    }

    AssetObjectManager = new FAssetObjectManager(
        new FAssetCacheManager,
        new RHI::D3D11::FD3D11DynamicRHI(Renderer->GetRHI().GetDevice(),
                                         Renderer->GetRHI().GetDeviceContext()));

     Viewer->SetRuntimeServices(&Renderer->GetRHI(),
                                AssetObjectManager->GetDynamicRHI(),
                                AssetObjectManager);

    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO();
#ifdef IMGUI_HAS_DOCK
    IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
    ImGui_ImplWin32_Init((void*)WindowHandle);
    ImGui_ImplDX11_Init(Renderer->GetRHI().GetDevice(), Renderer->GetRHI().GetDeviceContext());

    if (Engine::ApplicationCore::FWindowsApplication* WindowsApplication = GetWindowsApplication())
    {
        WindowsApplication->SetMessageHandler(&FViewerEngineLoop::HandleViewerMessage, this);
    }

    CachedWindowWidth = Application->GetWindowWidth();
    CachedWindowHeight = Application->GetWindowHeight();
    Viewer->OnWindowResized(static_cast<float>(CachedWindowWidth),
                            static_cast<float>(CachedWindowHeight));

    InitializeForTime();

    return true;
}

int32 FViewerEngineLoop::Run()
{
    while (!bIsExit)
    {
        if (bIsExit)
        {
            break;
        }

        Tick();
    }

    return 0;
}

void FViewerEngineLoop::ShutDown()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (Viewer != nullptr)
    {
        Viewer->SetRuntimeServices(nullptr, nullptr, nullptr);
    }

    delete AssetObjectManager;
    AssetObjectManager = nullptr;

    if (Renderer != nullptr)
    {
        Renderer->ShutdownModule();
        delete Renderer;
        Renderer = nullptr;
    }

    if (Viewer != nullptr)
    {
        Viewer->Release();
        delete Viewer;
        Viewer = nullptr;
    }

    if (Application != nullptr)
    {
        Application->DestroyApplicationWindow();
        delete Application;
        Application = nullptr;
    }

    delete InputSystem;
    InputSystem = nullptr;
}

void FViewerEngineLoop::ProcessSystemMessages()
{
    if (InputSystem != nullptr)
    {
        InputSystem->BeginFrame();
    }
    Application->PumpMessages();
}

void FViewerEngineLoop::Tick()
{
    ProcessSystemMessages();
    if (Application->IsExitRequested())
    {
        bIsExit = true;
        return;
    }

    RunFrameOnce();
}

void FViewerEngineLoop::InitializeForTime()
{
    PrevTime = FPlatformTime::Seconds();
    DeltaTime = 0.0f;
}

bool FViewerEngineLoop::HandleViewerMessage(HWND HWnd, UINT Message, WPARAM WParam, LPARAM LParam,
                                            LRESULT& OutResult, void* UserData)
{
    FViewerEngineLoop* ViewerEngineLoop = static_cast<FViewerEngineLoop*>(UserData);
    if (ViewerEngineLoop == nullptr)
    {
        return false;
    }

    const LRESULT ImGuiResult = ImGui_ImplWin32_WndProcHandler(HWnd, Message, WParam, LParam);
    if (ImGuiResult != 0)
    {
        OutResult = ImGuiResult;
        return true;
    }

    if (ImGui::GetCurrentContext() == nullptr)
    {
        return false;
    }

    OutResult = DefWindowProc(HWnd, Message, WParam, LParam);
}

bool FViewerEngineLoop::HandleWindowResize()
{
    const int32 CurrentWindowWidth = Application->GetWindowWidth();
    const int32 CurrentWindowHeight = Application->GetWindowHeight();
    if (CurrentWindowWidth == CachedWindowWidth && CurrentWindowHeight == CachedWindowHeight)
    {
        return false;
    }

    CachedWindowWidth = CurrentWindowWidth;
    CachedWindowHeight = CurrentWindowHeight;

    if (Renderer != nullptr)
    {
        Renderer->OnWindowResized(CurrentWindowWidth, CurrentWindowHeight);
    }

    if (Viewer != nullptr)
    {
        Viewer->OnWindowResized(static_cast<float>(CurrentWindowWidth),
                                static_cast<float>(CurrentWindowHeight));
    }

    return true;
}

bool FViewerEngineLoop::RunFrameOnce()
{
    HandleWindowResize();
    return RunFrameOnceWithoutResize();
}

bool FViewerEngineLoop::RunFrameOnceWithoutResize()
{
    if (bIsRenderingDuringSizeMove)
    {
        return false;
    }

    if (Application == nullptr || Viewer == nullptr || Renderer == nullptr ||
        InputSystem == nullptr)
    {
        return false;
    }

    if (Application->IsExitRequested())
    {
        bIsExit = true;
        return false;
    }

    bIsRenderingDuringSizeMove = true;

    UpdateFrameTiming();

    Viewer->Tick(DeltaTime, InputSystem);

    Renderer->BeginFrame();

    // Viewer에서 Viewport, RenderData 등은 Viewer 내부에서 관리한다고 가정
    Renderer->SetViewport(Viewer->GetSceneView()->GetViewport());
    Renderer->Render(Viewer->GetSceneRenderData());

    Viewer->DrawPanel(static_cast<HWND>(Application->GetNativeWindowHandle()));
    Renderer->EndFrame();

    bIsRenderingDuringSizeMove = false;
    return true;
}

void FViewerEngineLoop::UpdateFrameTiming()
{
    const double CurrentTime = FPlatformTime::Seconds();
    double       RawDeltaTime = CurrentTime - PrevTime;
    PrevTime = CurrentTime;

    if (RawDeltaTime < (1.0 / 1000.0))
    {
        RawDeltaTime = 1.0 / 1000.0;
    }
    else if (RawDeltaTime > (1.0 / 15.0))
    {
        RawDeltaTime = 1.0 / 15.0;
    }

    DeltaTime = static_cast<float>(RawDeltaTime);
    MainLoopFPS = static_cast<float>(1.0 / RawDeltaTime);
}

Engine::ApplicationCore::FWindowsApplication* FViewerEngineLoop::GetWindowsApplication() const
{
#if defined(_WIN32)
    return static_cast<Engine::ApplicationCore::FWindowsApplication*>(Application);
#else
    return nullptr;
#endif
}