#pragma once
#include "D3D11RHI.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/RenderState/RenderStateManager.h"
#include "Renderer/Shader/ShaderManager.h"
#include <functional>
#include <memory>

#include "Renderer/EditorRenderData.h"

// === Forward Declaraction
struct FRenderCommandQueue;
using FGUICallback = std::function<void()>;
class UMaterial;
// === Forward Declaraction


class ENGINE_API FGeneralRenderer
{
public:
    FGeneralRenderer(HWND InHwnd, int32 InWidth, int32 InHeight);
    ~FGeneralRenderer();
    
    bool Initialize(HWND InHwnd, int32 Width, int32 Height);
    void BeginFrame();
    void EndFrame();        // 스왑체인, 콜백 등 담당
    void Release();
    bool IsOccluded();
    void OnResize(int32 NewWidth, int32 NewHeight);
    
    void SetSceneRenderTarget(ID3D11RenderTargetView* InRenderTargetView, ID3D11DepthStencilView* InDepthStencilView, const D3D11_VIEWPORT& InViewport);
    void ClearSceneRenderTarget();
    void SetVSync(bool bEnable) { RHI.SetVSyncEnabled(bEnable); }
    bool IsVSyncEnabled() const { return RHI.IsVSyncEnabled(); }
    
    void SubmitCommands(const FRenderCommandQueue& Queue);
    void AddCommand(const FRenderCommand& Command);
    void ClearCommandList();
    void ExecuteCommands();
    
    bool Pick(int32 MouseX, int32 MouseY, uint32& OutPickId);

    // ─── GUI 및 콜백 ───
    /** ImGui 등 외부 GUI 시스템 연동용 콜백 */
    void SetGUICallbacks(FGUICallback InInit, FGUICallback InShutdown, FGUICallback InNewFrame, FGUICallback InRender, FGUICallback InPostPresent = nullptr);
    void ClearViewportCallbacks();
    void SetGUIUpdateCallback(FGUICallback InUpdate);
    
    static UMaterial* GetDefaultMaterial();
    static UMaterial* GetDefaultSpriteMaterial();
    UMaterial* GetLineMaterial() const { return AABBMaterial; }
    std::unique_ptr<CRenderStateManager>& GetRenderStateManager() { return RenderStateManager; }
    ID3D11Device* GetDevice() const { return RHI.GetDevice(); }
    ID3D11DeviceContext* GetDeviceContext() const { return RHI.GetDeviceContext(); }
    ID3D11RenderTargetView* GetRenderTargetView() const { return RHI.GetBackBufferRTV(); }
    IDXGISwapChain* GetSwapChain() const { return RHI.GetSwapChain(); };
    HWND GetHwnd() const { return Hwnd; }

    FVector GetCameraPosition() const;
    FD3D11RHI& GetRHI() { return RHI; }
    
    const FGizmoResources& GetGizmoResources() const { return GizmoResources; }
    
private:
    bool InitializeDefaultMaterial();
    
    void SetConstantBuffers();
    bool CreateConstantBuffers();
    void UpdateFrameConstantBuffer();
    void UpdateObjectConstantBuffer(const FMatrix& WorldMatrix, uint32 ObjectId = 0, 
                                    FVector2 UVOffset = {0,0},
                                    const FVector4& MultiplyColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f),
                                    const FVector4& AdditiveColor = FVector4(0.0f, 0.0f, 0.0f, 0.0f)
                                    );
    void ClearDepthBuffer();

    void ExecuteRenderPass(ERenderLayer InRenderLayer);
    void BindMaterial(ID3D11DeviceContext* DeviceContext, FRenderCommand Cmd);
    void BindRenderState(FRenderCommand Cmd);
    void DrawAllAABBLines(ERenderLayer InRenderLayer);
    
    /** AABB 전용 리소스 초기화 */
    void InitializeAABBResources();

    /** 기즈모 전용 리소스 초기화 */
    void InitializeGizmoResources();

    /** 픽킹 리소스 */
    bool CreatePickResources(int32 Width, int32 Height);
    void ReleasePickResources();
    bool ReadBackMousePixel(int32 MouseX, int32 MouseY, uint32& OutObjectId);
    
private:
    FD3D11RHI RHI;

    std::unique_ptr<CRenderStateManager> RenderStateManager = nullptr;
    HWND Hwnd = nullptr;
    
    ID3D11Buffer* FrameConstantBuffer = nullptr;
    ID3D11Buffer* ObjectConstantBuffer = nullptr;

    // ─── 픽킹 관련 리소스 ───
    ID3D11Texture2D*        PickColorTexture = nullptr;
    ID3D11RenderTargetView* PickRTV = nullptr;
    ID3D11Texture2D*        PickDepthTexture = nullptr;
    ID3D11DepthStencilView* PickDSV = nullptr;
    ID3D11Texture2D*        ReadbackTexture = nullptr;

    ID3D11VertexShader* PickVertexShader = nullptr;
    ID3D11PixelShader*  PickPixelShader = nullptr;
    ID3D11InputLayout*  PickInputLayout = nullptr;
    // ───────────────────────
    
    FMatrix ViewMatrix;
    FMatrix ProjectionMatrix;
    
    ID3D11RenderTargetView* SceneRenderTargetView = nullptr;
    ID3D11DepthStencilView* SceneDepthStencilView = nullptr;
    D3D11_VIEWPORT SceneViewport = {};
    
    bool bUseSceneRenderTargetOverride = false;
    bool bSwapChainOccluded = false;
    
    TArray<FRenderCommand> CommandList;
    FGUICallback GUIInit;
    FGUICallback GUIShutdown;
    FGUICallback GUINewFrame;
    FGUICallback GUIUpdate;
    FGUICallback GUIRender;
    FGUICallback GUIPostPresent;
    // FPostRenderCallback PostRenderCallback;
    
    /** 기본 공유 리소스 */
    static UMaterial* DefaultMaterial;
    static UMaterial* DefaultSpriteMaterial;
    UMaterial* DefaultTextureMaterial;

    std::shared_ptr<FVertexShader> DefaultMeshVS;
    std::shared_ptr<FPixelShader>  DefaultMeshPS;
    std::shared_ptr<FVertexShader> DefaultLineVS;
    std::shared_ptr<FPixelShader>  DefaultLinePS;
    
    /** AABB 전용 리소스 */
    std::shared_ptr<FMeshData> AABBMeshData;
    UMaterial* AABBMaterial;

    /** 기즈모 전용 리소스 */
    FGizmoResources GizmoResources;
    
    ID3D11SamplerState* NormalSampler = nullptr;
    
public:
    CShaderManager ShaderManager;
};
