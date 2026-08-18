#include "Viewport/EditorViewportClient.h"

#include "ApplicationCore/Input/InputRouter.h"
#include "Editor/EditorContext.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/World.h"
#include "Engine/Game/Actor.h"
#include "Engine/Component/Mesh/LineBatchComponent.h"

#include "imgui.h"
#include "Engine/EngineStatics.h"
#include "Renderer/RendererModule.h"
#include "Renderer/D3D11/GeneralRenderer.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/SceneRenderData.h"

void FEditorViewportClient::Create()
{
    InputRouter = new Engine::ApplicationCore::FInputRouter();
    InputRouter->AddContext(&ViewportInputContext);
    InputRouter->AddContext(&SelectionInputContext);
    InputRouter->AddContext(&GizmoInputContext);
    GizmoController.SetViewportClient(this);
    GizmoController.SetViewportSelectionController(&SelectionController);
    SelectionController.SetViewportClient(this);

    NavigationController.SetCamera(&ViewportCamera);
    NavigationController.SetSelectionController(&SelectionController);
    SelectionController.SetCamera(&ViewportCamera);
    GizmoController.SetCamera(&ViewportCamera);
    GizmoInputContext.SetNavigationController(&NavigationController);

    ViewportCamera.SetProjectionType(EViewportProjectionType::Perspective);
    ViewportCamera.SetFOV(3.141592f * 0.5f);
    ViewportCamera.SetNearPlane(0.1f);
    ViewportCamera.SetFarPlane(2000.0f);
    ViewportCamera.SetLocation(FVector(-20.0f, 1.0f, 10.0f));
    ViewportCamera.SetRotation(FRotator(0.0f, 0.0f, 0.0f));

    GridLineBatcher = new Engine::Component::ULineBatchComponent();
}

void FEditorViewportClient::Release()
{
    if (InputRouter)
    {
        delete InputRouter;
        InputRouter = nullptr;
    }

    if (GridLineBatcher)
    {
        delete GridLineBatcher;
        GridLineBatcher = nullptr;
    }
}

void FEditorViewportClient::Initialize(FWorld* World, uint32 ViewportWidth, uint32 ViewportHeight)
{
    SetWorld(World);

    ViewportCamera.OnResize(ViewportWidth, ViewportHeight);

    FScene* ActiveScene = (World != nullptr) ? World->GetActiveScene() : nullptr;
    SelectionController.SetActors(ActiveScene != nullptr ? ActiveScene->GetActors() : nullptr);
    SelectionController.SetCamera(&ViewportCamera);
    SelectionController.SetViewportSize(ViewportWidth, ViewportHeight);
}

void FEditorViewportClient::Tick(float DeltaTime, const Engine::ApplicationCore::FInputState& State)
{
    ViewportInputContext.SetDeltaTime(DeltaTime);

    if (InputRouter)
    {
        InputRouter->Tick(State);
    }

    if (CurWorld != nullptr)
    {
        FScene* ActiveScene = CurWorld->GetActiveScene();
        SelectionController.SetActors(ActiveScene != nullptr ? ActiveScene->GetActors() : nullptr);
    }
    else
    {
        SelectionController.SetActors(nullptr);
    }

    SelectionController.SyncSelectionFromContext();
    NavigationController.Tick(DeltaTime);
}

void FEditorViewportClient::Draw() {}

void FEditorViewportClient::HandleInputEvent(const Engine::ApplicationCore::FInputEvent& Event,
                                             const Engine::ApplicationCore::FInputState& State)
{
    if (InputRouter)
    {
        InputRouter->RouteEvent(Event, State);
    }
}

void FEditorViewportClient::HandleGizmoRenderCommand(FEditorRenderData& OutEditorRenderData, FSceneRenderData& OutSceneRenderData)
{
    if (!SelectionController.GetSelectedActors().empty())
    {
        OutEditorRenderData.Gizmo.GizmoType = GizmoController.GetGizmoType();
        OutEditorRenderData.Gizmo.Highlight = GizmoController.GetGizmoHighlight();
        GizmoController.SetSelectedActor(SelectionController.GetSelectedActors().back());
        if (GizmoController.bIsWorldMode && GizmoController.GetGizmoType() != EGizmoType::Scaling)
        {
            FVector RelativeLocation{
                GizmoController.GetSelectedActor()->GetRootComponent()->GetRelativeLocation()};
            OutEditorRenderData.Gizmo.Frame = FMatrix::MakeTranslation(RelativeLocation);
        }
        else
        {
            OutEditorRenderData.Gizmo.Frame =
                GizmoController.GetSelectedActor()->GetRootComponent()->GetRelativeMatrixNoScale();
        }
        GizmoController.GizmoScale =
            (ViewportCamera.GetLocation() -
             GizmoController.GetSelectedActor()->GetRootComponent()->GetRelativeLocation())
            .Size() /
            10.f;
        //  Size 여기서 조정
        OutEditorRenderData.Gizmo.Scale = GizmoController.GizmoScale;
        GizmoController.bIsDrawed = OutEditorRenderData.bShowGizmo;

        // Gizmo Render Commands
        if (OutEditorRenderData.bShowGizmo && OutEditorRenderData.Gizmo.GizmoType != EGizmoType::None)
        {
            if (SelectionController.GetEditorContext() && SelectionController.GetEditorContext()->Renderer)
            {
                FGeneralRenderer* GeneralRenderer = SelectionController.GetEditorContext()->Renderer->GetGeneralRenderer();
                if (GeneralRenderer)
                {
                    const auto& GizmoRes = GeneralRenderer->GetGizmoResources();
                    const auto& GizmoDraw = OutEditorRenderData.Gizmo;
                    FMatrix     World = FMatrix::MakeScale(GizmoDraw.Scale) * GizmoDraw.Frame;

                    const TArray<FGizmoMeshPart>* Parts = nullptr;
                    switch (GizmoDraw.GizmoType)
                    {
                    case EGizmoType::Translation: Parts = &GizmoRes.TranslationParts; break;
                    case EGizmoType::Rotation:    Parts = &GizmoRes.RotationParts;    break;
                    case EGizmoType::Scaling:     Parts = &GizmoRes.ScaleParts;       break;
                    }

                    if (Parts)
                    {
                        for (const auto& Part : *Parts)
                        {
                            FRenderCommand Cmd;
                            Cmd.MeshData = Part.MeshData.get();
                            Cmd.WorldMatrix = World;
                            Cmd.Material = FGeneralRenderer::GetDefaultMaterial();
                            Cmd.RenderLayer = ERenderLayer::Overlay;
                            Cmd.ObjectId = Part.PickId;

                            // Highlight Check
                            EGizmoType DecodedType;
                            EAxis      DecodedAxis;
                            bool       bIsHovered = false;
                            if (PickId::DecodeGizmoPart(Part.PickId, DecodedType, DecodedAxis))
                            {
                                if (DecodedAxis == EAxis::Center)
                                    bIsHovered = (GizmoDraw.Highlight == EGizmoHighlight::Center);
                                else if (DecodedAxis == EAxis::X)
                                    bIsHovered = (GizmoDraw.Highlight == EGizmoHighlight::X);
                                else if (DecodedAxis == EAxis::Y)
                                    bIsHovered = (GizmoDraw.Highlight == EGizmoHighlight::Y);
                                else if (DecodedAxis == EAxis::Z)
                                    bIsHovered = (GizmoDraw.Highlight == EGizmoHighlight::Z);
                            }
                            
                            if (bIsHovered)
                            {
                                // Solid Yellow
                                Cmd.MultiplyColor = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
                                Cmd.AdditiveColor = FVector4(1.0f, 1.0f, 0.0f, 1.0f);
                            }
                            else
                            {
                                Cmd.MultiplyColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
                                Cmd.AdditiveColor = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
                            }
                            
                            Cmd.DepthStencilOption.DepthEnable = true;
                            Cmd.DepthStencilOption.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
                            Cmd.DepthStencilOption.DepthFunc = D3D11_COMPARISON_ALWAYS;
                            Cmd.RasterizerOption.CullMode = D3D11_CULL_NONE;

                            OutSceneRenderData.RenderCommands.push_back(Cmd);
                        }
                    }
                }
            }
        }
    }
    else
    {
        OutEditorRenderData.Gizmo.GizmoType = EGizmoType::None;
        OutEditorRenderData.Gizmo.Highlight = EGizmoHighlight::None;
        OutEditorRenderData.Gizmo.Frame = FMatrix::Identity;
        OutEditorRenderData.Gizmo.Scale = 1.0f;
        GizmoController.SetSelectedActor(nullptr);
        GizmoController.bIsDrawed = false;
    }
}

void FEditorViewportClient::BuildRenderData(FEditorRenderData& OutEditorRenderData,
                                            FSceneRenderData&  OutSceneRenderData,
                                            EEditorShowFlags   InShowFlags)
{
    // SceneView 업데이트
    SceneView.SetViewMatrix(ViewportCamera.GetViewMatrix());
    SceneView.SetProjectionMatrix(ViewportCamera.GetProjectionMatrix());
    SceneView.SetViewLocation(ViewportCamera.GetLocation());
    SceneView.SetClipPlanes(ViewportCamera.GetNearPlane(), ViewportCamera.GetFarPlane());
    SceneView.OnResize({0, 0, (int32)ViewportCamera.GetWidth(), (int32)ViewportCamera.GetHeight()});
    OutSceneRenderData.SceneView = &SceneView;
    
    switch(RenderSetting.GetViewMode())
    {
    case EViewModeIndex::VMI_Lit:
    case EViewModeIndex::VMI_Unlit:
        break;      // Do nothing
    case EViewModeIndex::VMI_Wireframe:
        for (auto& RC : OutSceneRenderData.RenderCommands)
        {
            if (RC.bIgnoreWireFrame) 
                continue;
            RC.RasterizerOption.FillMode = D3D11_FILL_WIREFRAME;
        }
        break;
    }

    // EditorView 업데이트
    OutEditorRenderData.bShowGrid = 
                                IsFlagSet(InShowFlags, EEditorShowFlags::SF_Grid);
    OutEditorRenderData.bShowWorldAxes = 
                                IsFlagSet(InShowFlags, EEditorShowFlags::SF_WorldAxes);
    OutEditorRenderData.bShowSelectionOutline =
                                IsFlagSet(InShowFlags, EEditorShowFlags::SF_SelectionOutline);
    OutEditorRenderData.bShowGizmo = 
                                IsFlagSet(InShowFlags, EEditorShowFlags::SF_Gizmo);

    // 기즈모 렌더 커맨드 생성
    // 260401 NOTE: 기즈모 '정보'는 OutEditorRenderData, '렌더 커맨드'는 OutSceneRenderData에 나눠져있음... 
    // \(^o^)/
    HandleGizmoRenderCommand(OutEditorRenderData, OutSceneRenderData);

    // 그리드 및 월드 축 렌더 커맨드 생성
    if (OutEditorRenderData.bShowGrid || OutEditorRenderData.bShowWorldAxes)
    {
        DrawWorldGrid(OutEditorRenderData, OutSceneRenderData);
    }
}

void FEditorViewportClient::DrawWorldGrid(FEditorRenderData& OutEditorRenderData,
                                          FSceneRenderData&  OutSceneRenderData)
{
    if (!GridLineBatcher)
    {
        return;
    }
    
    const float  PersistentLifeTime = -1.0f;

    GridLineBatcher->ClearLines();
    
    if (OutEditorRenderData.bShowGrid)
    {
        const float GridSpacing = UEngineStatics::GridSpacing;
        const FColor GridColor(0.3f, 0.3f, 0.3f, 1.0f);

        // 카메라 위치를 기준으로 그리드 범위를 결정합니다.
        const FVector CamLoc = ViewportCamera.GetLocation();
        const float   MaxGridRange = 2000.0f; // 카메라로부터 그리드가 그려질 반경
        const int32   LineCount = static_cast<int32>(MaxGridRange / GridSpacing);
    
        const EViewportProjectionType ProjType = ViewportCamera.GetProjectionType();
        const EViewportOrthographicType OrthoType = ViewportCamera.GetOrthographicType();

        if (ProjType == EViewportProjectionType::Perspective || OrthoType == EViewportOrthographicType::Top ||
            OrthoType == EViewportOrthographicType::Bottom)
        {
            // XY 평면 그리드: 카메라 위치 주변으로 스냅된 중심 계산
            float SnapX = std::floor(CamLoc.X / GridSpacing) * GridSpacing;
            float SnapY = std::floor(CamLoc.Y / GridSpacing) * GridSpacing;

            float MinX = SnapX - LineCount * GridSpacing;
            float MaxX = SnapX + LineCount * GridSpacing;
            float MinY = SnapY - LineCount * GridSpacing;
            float MaxY = SnapY + LineCount * GridSpacing;

            // X축에 평행한 선들 (Y축 값을 변화시키며 그림)
            for (int32 i = -LineCount; i <= LineCount; ++i)
            {
                float y = SnapY + i * GridSpacing;
                if (OutEditorRenderData.bShowWorldAxes && std::abs(y) < 0.01f) 
                    continue; // 월드 축과 겹치는 부분 제외
                GridLineBatcher->AddLine(FVector(MinX, y, 0.0f), FVector(MaxX, y, 0.0f), GridColor, PersistentLifeTime);
            }

            // Y축에 평행한 선들 (X축 값을 변화시키며 그림)
            for (int32 i = -LineCount; i <= LineCount; ++i)
            {
                float x = SnapX + i * GridSpacing;
                if (OutEditorRenderData.bShowWorldAxes && std::abs(x) < 0.01f) 
                    continue; // 월드 축과 겹치는 부분 제외
                GridLineBatcher->AddLine(FVector(x, MinY, 0.0f), FVector(x, MaxY, 0.0f), GridColor, PersistentLifeTime);
            }
        }
        else if (OrthoType == EViewportOrthographicType::Front || OrthoType == EViewportOrthographicType::Back)
        {
            // YZ 평면 그리드
            float SnapY = std::floor(CamLoc.Y / GridSpacing) * GridSpacing;
            float SnapZ = std::floor(CamLoc.Z / GridSpacing) * GridSpacing;

            float MinY = SnapY - LineCount * GridSpacing;
            float MaxY = SnapY + LineCount * GridSpacing;
            float MinZ = SnapZ - LineCount * GridSpacing;
            float MaxZ = SnapZ + LineCount * GridSpacing;

            for (int32 i = -LineCount; i <= LineCount; ++i)
            {
                float z = SnapZ + i * GridSpacing;
                if (OutEditorRenderData.bShowWorldAxes && std::abs(z) < 0.01f) 
                    continue;
                GridLineBatcher->AddLine(FVector(0.0f, MinY, z), FVector(0.0f, MaxY, z), GridColor, PersistentLifeTime);
            }

            for (int32 i = -LineCount; i <= LineCount; ++i)
            {
                float y = SnapY + i * GridSpacing;
                if (OutEditorRenderData.bShowWorldAxes && std::abs(y) < 0.01f) 
                    continue;
                GridLineBatcher->AddLine(FVector(0.0f, y, MinZ), FVector(0.0f, y, MaxZ), GridColor, PersistentLifeTime);
            }
        }
        else if (OrthoType == EViewportOrthographicType::Left || OrthoType == EViewportOrthographicType::Right)
        {
            // XZ 평면 그리드
            float SnapX = std::floor(CamLoc.X / GridSpacing) * GridSpacing;
            float SnapZ = std::floor(CamLoc.Z / GridSpacing) * GridSpacing;

            float MinX = SnapX - LineCount * GridSpacing;
            float MaxX = SnapX + LineCount * GridSpacing;
            float MinZ = SnapZ - LineCount * GridSpacing;
            float MaxZ = SnapZ + LineCount * GridSpacing;

            for (int32 i = -LineCount; i <= LineCount; ++i)
            {
                float z = SnapZ + i * GridSpacing;
                if (OutEditorRenderData.bShowWorldAxes && std::abs(z) < 0.01f) 
                    continue;
                GridLineBatcher->AddLine(FVector(MinX, 0.0f, z), FVector(MaxX, 0.0f, z), GridColor, PersistentLifeTime);
            }

            for (int32 i = -LineCount; i <= LineCount; ++i)
            {
                float x = SnapX + i * GridSpacing;
                if (OutEditorRenderData.bShowWorldAxes && std::abs(x) < 0.01f) 
                    continue;
                GridLineBatcher->AddLine(FVector(x, 0.0f, MinZ), FVector(x, 0.0f, MaxZ), GridColor, PersistentLifeTime);
            }
        }
    }

    // 월드 축
    if (OutEditorRenderData.bShowWorldAxes)
    {
        const float AxisLen = 10000.0f; // 축은 충분히 길게 그림
        GridLineBatcher->AddLine(FVector(-AxisLen, 0.0f, 0.0f), FVector(AxisLen, 0.0f, 0.0f), FColor::Red(),
                                 PersistentLifeTime);
        GridLineBatcher->AddLine(FVector(0.0f, -AxisLen, 0.0f), FVector(0.0f, AxisLen, 0.0f), FColor::Green(),
                                 PersistentLifeTime);
        GridLineBatcher->AddLine(FVector(0.0f, 0.0f, -AxisLen), FVector(0.0f, 0.0f, AxisLen), FColor::Blue(),
                                 PersistentLifeTime);
    }

    // 그리드 라인배처의 렌더 데이터를 수집하여 OutSceneRenderData에 추가
    GridLineBatcher->CollectRenderData(OutSceneRenderData, ESceneShowFlags::SF_Primitives);
}

void FEditorViewportClient::OnResize(uint32 Width, uint32 Height)
{
    ViewportCamera.OnResize(Width, Height);
    SelectionController.SetViewportSize(Width, Height);
}

void FEditorViewportClient::SetEditorContext(FEditorContext* InContext)
{
    SelectionController.SetEditorContext(InContext);
}

void FEditorViewportClient::SetWorld(FWorld* InWorld)
{
    CurWorld = InWorld;
    FScene* ActiveScene = (CurWorld != nullptr) ? CurWorld->GetActiveScene() : nullptr;
    SelectionController.SetActors(ActiveScene != nullptr ? ActiveScene->GetActors() : nullptr);
}

void FEditorViewportClient::SyncSelectionFromContext()
{
    SelectionController.SyncSelectionFromContext();
}

void FEditorViewportClient::DrawViewportOverlay()
{
    if (!SelectionController.IsDraggingSelection())
    {
        return;
    }

    int32 StartX, StartY, EndX, EndY;
    SelectionController.GetSelectionRect(StartX, StartY, EndX, EndY);

    const float MinX = (float)std::min(StartX, EndX);
    const float MinY = (float)std::min(StartY, EndY);
    const float MaxX = (float)std::max(StartX, EndX);
    const float MaxY = (float)std::max(StartY, EndY);

    ImDrawList* DrawList = ImGui::GetForegroundDrawList();
    DrawList->AddRectFilled(ImVec2(MinX, MinY), ImVec2(MaxX, MaxY), IM_COL32(80, 140, 255, 40));
    DrawList->AddRect(ImVec2(MinX, MinY), ImVec2(MaxX, MaxY), IM_COL32(80, 140, 255, 255), 0.0f, 0,
                      1.5f);
}
