#pragma once

#include "Navigation/ViewportNavigationController.h"
#include "Selection/ViewportSelectionController.h"
#include "Gizmo/ViewportGizmoController.h"
#include "Interaction/ViewportInteractionState.h"
#include "RenderSetting/ViewportRenderSetting.h"
#include "Engine/ViewPort/ViewportClient.h"
#include "Engine/Scene/World.h"
#include "Input/NavigationInputContext.h"
#include "Input/SelectionInputContext.h"
#include "Input/GizmoInputContext.h"
#include "Renderer/EditorRenderData.h"
#include "Renderer/Types/EditorShowFlags.h"
#include "Renderer/SceneView.h"

namespace Engine::Component { class ULineBatchComponent; }

class FEditorViewportClient : public Engine::Viewport::IViewportClient
{
  public:
    FEditorViewportClient() = default;
    ~FEditorViewportClient() override = default;

    void Create() override;
    void Release() override;

    void Initialize(FWorld* World, uint32 ViewportWidth, uint32 ViewportHeight) override;

    void Tick(float DeltaTime, const Engine::ApplicationCore::FInputState& State) override;
    void Draw() override;

    void HandleInputEvent(const Engine::ApplicationCore::FInputEvent& Event,
                          const Engine::ApplicationCore::FInputState& State) override;
    void HandleGizmoRenderCommand(FEditorRenderData& OutEditorRenderData,
                                  FSceneRenderData&  OutSceneRenderData);

    void BuildRenderData(FEditorRenderData& OutEditorRenderData,
                         FSceneRenderData&  OutSceneRenderData, EEditorShowFlags InShowFlags);

    void OnResize(uint32 InWidth, uint32 InHeight);
    void SetEditorContext(FEditorContext* InContext);
    void SetWorld(FWorld* InWorld);
    void SyncSelectionFromContext();

    FViewportNavigationController&       GetNavigationController() { return NavigationController; }
    const FViewportNavigationController& GetNavigationController() const
    {
        return NavigationController;
    }
    FViewportSelectionController& GetSelectionController() { return SelectionController; }
    FViewportGizmoController&     GetGizmoController() { return GizmoController; }
    FViewportInteractionState&    GetInteractionState() { return InteractionState; }
    FViewportRenderSetting&       GetRenderSetting() { return RenderSetting; }
    const FViewportRenderSetting& GetRenderSetting() const { return RenderSetting; }

    void DrawViewportOverlay();

    FViewportCamera& GetCamera() { return ViewportCamera; }
    using FPickCallback = std::function<FPickResult(int32, int32)>;
    FPickCallback OnPickRequested;

    FPickResult PickAt(int32 MouseX, int32 MouseY) const
    {
        if (OnPickRequested)
        {
            return OnPickRequested(MouseX, MouseY);
        }
        return FPickResult{};
    }

  private:
    void DrawOutline();
    void DrawWorldGrid(FEditorRenderData& OutEditorRenderData,
                       FSceneRenderData&  OutSceneRenderData);

  private:
    FWorld* CurWorld = nullptr;

    FViewportCamera ViewportCamera;
    FSceneView      SceneView;

    FViewportNavigationController NavigationController;
    FViewportSelectionController  SelectionController;
    FViewportGizmoController      GizmoController;
    FViewportInteractionState     InteractionState;
    FViewportRenderSetting        RenderSetting;

    FNavigationInputContext ViewportInputContext{&NavigationController};
    FSelectionInputContext  SelectionInputContext{&SelectionController};
    FGizmoInputContext      GizmoInputContext{&GizmoController};

    Engine::Component::ULineBatchComponent* GridLineBatcher = nullptr;
};
