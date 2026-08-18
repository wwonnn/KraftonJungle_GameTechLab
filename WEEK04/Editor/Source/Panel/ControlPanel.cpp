#include "ControlPanel.h"

#include "Camera/ViewportCamera.h"
#include "Core/Math/MathUtility.h"
#include "Editor/Editor.h"
#include "Editor/EditorContext.h"
#include "Engine/EngineStatics.h"
#include "Viewport/EditorViewportClient.h"
#include "imgui.h"
#include "IconsFontAwesome4.h"

namespace
{
    // Projection
    struct FProjectionButton
    {
        const char*               Label;
        EViewportProjectionType   ProjectionType;
        EViewportOrthographicType OrthoType;
        bool                      bIsOrtho;
    };

    constexpr FProjectionButton ProjectionTypeButtons[] = {
        {ICON_FA_CUBE        "Perspective", EViewportProjectionType::Perspective,
         EViewportOrthographicType::Top,    false},
        {ICON_FA_ARROW_UP    "Top",         EViewportProjectionType::Orthographic,
         EViewportOrthographicType::Top,    true},
        {ICON_FA_ARROW_DOWN  "Bottom",      EViewportProjectionType::Orthographic,
         EViewportOrthographicType::Bottom, true},
        {ICON_FA_ARROW_LEFT  "Left",        EViewportProjectionType::Orthographic,
         EViewportOrthographicType::Left,   true},
        {ICON_FA_ARROW_RIGHT "Right",       EViewportProjectionType::Orthographic,
         EViewportOrthographicType::Right,  true},
        {ICON_FA_SQUARE      "Front",       EViewportProjectionType::Orthographic,
         EViewportOrthographicType::Front,  true},
        {ICON_FA_SQUARE_O    "Back",        EViewportProjectionType::Orthographic,
         EViewportOrthographicType::Back,   true},
    };

    // View Mode
    struct FViewModeButton
    {
        const char*    Label;
        EViewModeIndex ViewMode;
    };

    constexpr FViewModeButton ViewModeButtons[] = {
        {ICON_FA_SUN_O   " Lit",       EViewModeIndex::VMI_Lit},
        {ICON_FA_MOON_O  " Unlit",     EViewModeIndex::VMI_Unlit},
        {ICON_FA_CODEPEN " Wireframe", EViewModeIndex::VMI_Wireframe},
    };

    // Layout
    struct FLayoutPreview
    {
        EViewportLayoutType Type;
        struct PanelRect
        {
            float x, y, w, h;
        };
        std::vector<PanelRect> Panels;
    };

    static const FLayoutPreview LayoutPreviews[] = 
    {
        {EViewportLayoutType::Single, {{0, 0, 1, 1}}},

        {EViewportLayoutType::_1l1, {{0, 0, .5f, 1}, {.5f, 0, .5f, 1}}},
        {EViewportLayoutType::_1_1, {{0, 0, 1, .5f}, {0, .5f, 1, .5f}}},

        {EViewportLayoutType::_1l2,
         {{0, 0, .5f, 1}, {.5f, 0, .5f, .5f}, {.5f, .5f, .5f, .5f}}},
        {EViewportLayoutType::_2l1,
         {{0, 0, .5f, .5f}, {0, .5f, .5f, .5f}, {.5f, 0, .5f, 1}}},
        {EViewportLayoutType::_1_2,
         {{0, 0, 1, .5f}, {0, .5f, .5f, .5f}, {.5f, .5f, .5f, .5f}}},
        {EViewportLayoutType::_2_1,
         {{0, 0, .5f, .5f}, {.5f, 0, .5f, .5f}, {0, .5f, 1, .5f}}},

        {EViewportLayoutType::_2X2,
         {{0, 0, .5f, .5f}, {.5f, 0, .5f, .5f}, {0, .5f, .5f, .5f}, {.5f, .5f, .5f, .5f}}},
        {EViewportLayoutType::_1l3,
         {{0, 0, .5f, 1}, {.5f, 0, .5f, .333f}, {.5f, .333f, .5f, .334f}, {.5f, .667f, .5f, .333f}}},
        {EViewportLayoutType::_3l1,
         {{0, 0, .5f, .333f}, {0, .333f, .5f, .334f}, {0, .667f, .5f, .333f}, {.5f, 0, .5f, 1}}},
        {EViewportLayoutType::_1_3,
         {{0, 0, 1, .5f}, {0, .5f, .333f, .5f}, {.333f, .5f, .334f, .5f}, {.667f, .5f, .333f, .5f}}},
        {EViewportLayoutType::_3_1,
         {{0, 0, .333f, .5f}, {.333f, 0, .334f, .5f}, {.667f, 0, .333f, .5f}, {0, .5f, 1, .5f}}},
    };

    void DrawVectorRow(const char* Label, FVector& Value, float Speed = 0.1f)
    {
        ImGui::PushID(Label);
        ImGui::TextUnformatted(Label);
        ImGui::SameLine(120.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::DragFloat3("##Value", Value.XYZ, Speed);
        ImGui::PopID();
    }

    void DrawRotatorRow(const char* Label, FRotator& Value, float Speed = 0.5f)
    {
        FVector EulerDegrees = Value.Euler();

        ImGui::PushID(Label);
        ImGui::TextUnformatted(Label);
        ImGui::SameLine(120.0f);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::DragFloat3("##Value", EulerDegrees.XYZ, Speed))
        {
            // Camera panel도 내부 Rotator 순서 대신 축 기반 X/Y/Z 회전으로 편집합니다.
            Value = FRotator::MakeFromEuler(EulerDegrees);
        }
        ImGui::PopID();
    }
} // namespace

const wchar_t* FControlPanel::GetPanelID() const
{
    return L"ControlPanel";
}

const wchar_t* FControlPanel::GetDisplayName() const
{
    return L"Control Panel";
}

void FControlPanel::Draw()
{
    FViewportRect Rect = GetContext()->Editor->GetViewportTab().GetViewport(ViewportIndex)->GetSceneView()->GetViewRect();
    
    ImGui::SetNextWindowPos(ImVec2(Rect.X, Rect.Y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(Rect.Width, 40.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.8f);

    ImGuiWindowFlags Flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavFocus |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDocking;

    const FString WindowId = "##Control Panel" + std::to_string(ViewportIndex);

    if (!ImGui::Begin(WindowId.c_str(), nullptr, Flags))
    {
        ImGui::End();
        return;
    }

    FViewportCamera* Camera = ResolveViewportCamera();
    if (Camera == nullptr)
    {
        DrawUnavailableState();
        ImGui::End();
        return;
    }

    DrawSectionButton(ICON_FA_ARROWS, "TransformPopup", [&]() { if (Camera) DrawTransformSection(*Camera);});
    ImGui::SameLine();
    DrawSectionButton(ICON_FA_VIDEO_CAMERA, "ProjectionPopup", [&]() { if (Camera) DrawProjectionSection(*Camera); });
    ImGui::SameLine();
    DrawSectionButton(ICON_FA_CUBE, "ViewModePopup", [&]() { DrawViewModeSection(); });
    ImGui::SameLine();
    DrawSectionButton(ICON_FA_EYE, "ShowFlagsPopup", [&]() { DrawShowFlagsSection(); });
    ImGui::SameLine();
    DrawSectionButton(ICON_FA_COG, "NavigationPopup", [&]() { DrawNavigationSection(); });
    ImGui::SameLine();
    DrawSectionButton(ICON_FA_GLOBE, "WorldPopup", [&]() { DrawWorldSection(); });
    ImGui::SameLine();
    DrawSectionButton(ICON_FA_TH_LARGE, "LayoutPopup", [&]() { DrawLayoutSection(); });
    ImGui::SameLine();

    ImGui::End();
}

FViewportCamera* FControlPanel::ResolveViewportCamera() const
{
    if (GetContext() == nullptr || GetContext()->Editor == nullptr)
    {
        return nullptr;
    }

    return &GetContext()
                ->Editor->GetViewportTab()
                .GetViewport(ViewportIndex)
                ->GetViewportClient()
                ->GetCamera();
}

void FControlPanel::DrawUnavailableState() const
{
    ImGui::TextUnformatted("Editor camera is unavailable.");
    ImGui::Spacing();
    ImGui::TextWrapped("The control panel requires an active editor viewport client.");
}

void FControlPanel::DrawTransformSection(FViewportCamera& Camera) const
{
    ImGui::TextUnformatted("Camera Transform");

    FVector Location = Camera.GetLocation();
    FRotator Rotation = Camera.GetRotation().Rotator();

    DrawVectorRow("Location", Location, 0.1f);
    DrawRotatorRow("Rotation", Rotation, 0.5f);

    if (Camera.GetLocation() != Location)
    {
        Camera.SetLocation(Location);
        GetContext()
            ->Editor->GetViewportTab()
            .GetViewport(ViewportIndex)
            ->GetViewportClient()
            ->GetNavigationController()
            .SetTargetLocation(Location);
    }

    if (!Camera.GetRotation().Rotator().Equals(Rotation))
    {
        Camera.SetRotation(Rotation);
    }
}

void FControlPanel::DrawProjectionSection(FViewportCamera& Camera) const
{
    ImGui::TextUnformatted("Projection");
    ImGui::SeparatorText("Perspective");

    const bool bIsPerspective = Camera.GetProjectionType() == EViewportProjectionType::Perspective;
    const EViewportOrthographicType CurrentOrthoType = Camera.GetOrthographicType();

    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
    for (int i = 0; i < IM_ARRAYSIZE(ProjectionTypeButtons); ++i)
    {
        if (i == 1)
            ImGui::SeparatorText("Orthographic");

        const bool bActive = ProjectionTypeButtons[i].bIsOrtho
                ? (!bIsPerspective && CurrentOrthoType == ProjectionTypeButtons[i].OrthoType)
                                 : bIsPerspective;

        if (bActive)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button(ProjectionTypeButtons[i].Label, ImVec2(120, 0)))
        {
            Camera.SetProjectionType(ProjectionTypeButtons[i].ProjectionType);
            if (ProjectionTypeButtons[i].bIsOrtho)
                Camera.SetOrthographicType(ProjectionTypeButtons[i].OrthoType);
            else
            {
                GetContext()
                    ->Editor->GetViewportTab()
                    .GetViewport(ViewportIndex)
                    ->GetViewportClient()
                    ->GetNavigationController()
                    .SetTargetLocation(Camera.GetPerspectiveInfo().Location);
            }
        }
        if (bActive)
            ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar();
    ImGui::Spacing();

    ImGui::SeparatorText("View");
    if (bIsPerspective)
    {
        float FOVDegrees = FMath::RadiansToDegrees(Camera.GetFOV());
        if (ImGui::SliderFloat("FOV", &FOVDegrees, 30.0f, 120.0f, "%.1f deg"))
        {
            Camera.SetFOV(FMath::DegreesToRadians(FOVDegrees));
        }
    }
    else
    {
        float OrthoHeight = Camera.GetOrthoHeight();
        if (ImGui::DragFloat("Ortho Height", &OrthoHeight, 0.1f, 1.0f, 1000.0f, "%.1f"))
        {
            Camera.SetOrthoHeight(FMath::Clamp(OrthoHeight, 1.0f, 1000.0f));
        }
    }

    ImGui::Spacing();
    ImGui::Text("Viewport: %u x %u", Camera.GetWidth(), Camera.GetHeight());
    ImGui::Text("Aspect Ratio: %.3f", Camera.GetAspectRatio());
}

void FControlPanel::DrawViewModeSection() const
{
    if (GetContext() == nullptr || GetContext()->Editor == nullptr)
    {
        return;
    }

    FViewportRenderSetting& RenderSetting = GetContext()
                                                ->Editor->GetViewportTab()
                                                .GetViewport(ViewportIndex)
                                                ->GetViewportClient()
                                                ->GetRenderSetting();

    ImGui::TextUnformatted("View Mode");

    const EViewModeIndex CurrentViewMode = RenderSetting.GetViewMode();

    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

    for (int i = 0; i < IM_ARRAYSIZE(ViewModeButtons); ++i)
    {
        const bool bActive = CurrentViewMode == ViewModeButtons[i].ViewMode;

        if (bActive)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button(ViewModeButtons[i].Label, ImVec2(120, 0)))
            RenderSetting.SetViewMode(ViewModeButtons[i].ViewMode);
        if (bActive)
            ImGui::PopStyleColor();
    }

    ImGui::PopStyleVar();
}

void FControlPanel::DrawLayoutSection() const 
{
    if (GetContext() == nullptr || GetContext()->Editor == nullptr)
    {
        return;
    }

    SEditorViewportTab& ViewportTab = GetContext()->Editor->GetViewportTab();
    EViewportLayoutType CurrentLayout = ViewportTab.GetCurrentLayoutType();

    ImGui::TextUnformatted("Layout");

    ImGui::Spacing();
    ImGui::SeparatorText("Single Pane");
    ImGui::Spacing();

    constexpr float ButtonSize = 32.0f;
    constexpr float ButtonGap = 5.0f;

    for (int i = 0; i < IM_ARRAYSIZE(LayoutPreviews); ++i)
    {
        const FLayoutPreview& Preview = LayoutPreviews[i];
        bool                  bSelected = (CurrentLayout == Preview.Type);

        ImVec2 CursorPos = ImGui::GetCursorScreenPos();

        constexpr float Pad = 3.0f, Gap = 1.5f, Rounding = 1.0f;

        // 색
        ImVec4 ColBg =
            bSelected ? ImVec4(0.10f, 0.18f, 0.30f, 1.0f) : ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
        ImVec4 ColPanel =
            bSelected ? ImVec4(0.16f, 0.30f, 0.50f, 1.0f) : ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
        ImVec4 ColBorder =
            bSelected ? ImVec4(0.25f, 0.45f, 0.75f, 1.0f) : ImVec4(0.60f, 0.60f, 0.60f, 1.0f);

        ImGui::PushID(i);
        ImGui::InvisibleButton("##L", ImVec2(ButtonSize, ButtonSize));
        bool bClicked = ImGui::IsItemClicked();
        bool bHovered = ImGui::IsItemHovered();
        ImGui::PopID();

        if (bHovered && !bSelected)
            ColBg = ImVec4(ColBg.x + 0.08f, ColBg.y + 0.08f, ColBg.z + 0.08f, 1.f);

        ImDrawList* DL = ImGui::GetWindowDrawList();
        DL->AddRectFilled(CursorPos, {CursorPos.x + ButtonSize, CursorPos.y + ButtonSize},
                          ImGui::ColorConvertFloat4ToU32(ColBg), 3.f);
        DL->AddRect(CursorPos, {CursorPos.x + ButtonSize, CursorPos.y + ButtonSize},
                    ImGui::ColorConvertFloat4ToU32(ColBorder), 3.f, 0, 1.2f);

        float IW = ButtonSize - Pad * 2, IH = ButtonSize - Pad * 2;
        for (const auto& P : Preview.Panels)
        {
            float px = CursorPos.x + Pad + P.x * IW + Gap;
            float py = CursorPos.y + Pad + P.y * IH + Gap;
            DL->AddRectFilled({px, py}, {px + P.w * IW - Gap * 2, py + P.h * IH - Gap * 2},
                              ImGui::ColorConvertFloat4ToU32(ColPanel), Rounding);
        }

        if (bClicked)
        {
            ViewportTab.SetLayout(Preview.Type);
            ImGui::CloseCurrentPopup();
        }

        // 줄바꿈 처리
        if (i == 0)
        {
            ImGui::SeparatorText("Two Panes");
            ImGui::Dummy({0, ButtonGap});
        }
        else if (i == 2)
        {
            ImGui::SeparatorText("Three Panes");
            ImGui::Dummy({0, ButtonGap});
        }
        else if (i == 6)
        {
            ImGui::SeparatorText("Four Panes");
            ImGui::Dummy({0, ButtonGap});
        }
        else
        {
            ImGui::SameLine(0, ButtonGap);
        }
    }
}

void FControlPanel::DrawShowFlagsSection() const
{
    if (GetContext() == nullptr || GetContext()->Editor == nullptr)
    {
        return;
    }

    FViewportRenderSetting& RenderSetting = GetContext()
                                                ->Editor->GetViewportTab()
                                                .GetViewport(ViewportIndex)
                                                ->GetViewportClient()
                                                ->GetRenderSetting();

    ImGui::TextUnformatted("Show Flags");

    bool bShowGrid = RenderSetting.IsGridVisible();
    if (ImGui::Checkbox("Editor Grid", &bShowGrid))
    {
        RenderSetting.SetGridVisible(bShowGrid);
    }

    bool bShowWorldAxes = RenderSetting.IsWorldAxesVisible();
    if (ImGui::Checkbox("Editor World Axes", &bShowWorldAxes))
    {
        RenderSetting.SetWorldAxesVisible(bShowWorldAxes);
    }

    bool bShowGizmo = RenderSetting.IsGizmoVisible();
    if (ImGui::Checkbox("Editor Gizmo", &bShowGizmo))
    {
        RenderSetting.SetGizmoVisible(bShowGizmo);
    }

    bool bShowSelectionOutline = RenderSetting.IsSelectionOutlineVisible();
    if (ImGui::Checkbox("Editor Selection Outline", &bShowSelectionOutline))
    {
        RenderSetting.SetSelectionOutlineVisible(bShowSelectionOutline);
    }

    bool bShowObjectLabels = RenderSetting.IsObjectLabelsVisible();
    if (ImGui::Checkbox("Editor Object Labels", &bShowObjectLabels))
    {
        RenderSetting.SetObjectLabelsVisible(bShowObjectLabels);
    }

    bool bShowUUID = RenderSetting.IsUUIDVisible();
    if (ImGui::Checkbox("Editor UUID Labels", &bShowUUID))
    {
        RenderSetting.SetUUIDVisible(bShowUUID);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    bool bShowScenePrimitives = RenderSetting.AreScenePrimitivesVisible();
    if (ImGui::Checkbox("Scene Primitives", &bShowScenePrimitives))
    {
        RenderSetting.SetScenePrimitivesVisible(bShowScenePrimitives);
    }

    bool bShowSceneSprites = RenderSetting.AreSceneSpritesVisible();
    if (ImGui::Checkbox("Scene Sprites", &bShowSceneSprites))
    {
        RenderSetting.SetSceneSpritesVisible(bShowSceneSprites);
    }

    bool bShowBillboardText = RenderSetting.AreBillboardTextVisible();
    if (ImGui::Checkbox("Scene Billboard Text", &bShowBillboardText))
    {
        RenderSetting.SetBillboardTextVisible(bShowBillboardText);
    }
}

void FControlPanel::DrawNavigationSection() const
{
    if (GetContext() == nullptr || GetContext()->Editor == nullptr)
    {
        return;
    }

    FViewportNavigationController& NavigationController = GetContext()
                                                              ->Editor->GetViewportTab()
                                                              .GetViewport(ViewportIndex)
                                                              ->GetViewportClient()
                                                              ->GetNavigationController();

    ImGui::TextUnformatted("Navigation");

    float MoveSpeed = NavigationController.GetMoveSpeed();
    if (ImGui::DragFloat("Move Speed", &MoveSpeed, 1.0f, 10.0f, 2000.0f, "%.1f"))
    {
        NavigationController.SetMoveSpeed(MoveSpeed);
    }

    float RotationSpeed = NavigationController.GetRotationSpeed();
    if (ImGui::DragFloat("Rotation Speed", &RotationSpeed, 0.01f, 0.01f, 10.0f, "%.2f"))
    {
        NavigationController.SetRotationSpeed(RotationSpeed);
    }
}

void FControlPanel::DrawWorldSection() const
{
    ImGui::TextUnformatted("World");

    float GridSpacing = UEngineStatics::GridSpacing;
    if (ImGui::DragFloat("Grid Spacing", &GridSpacing, 0.1f, 1.0f, 1000.0f, "%.1f"))
    {
        UEngineStatics::GridSpacing = FMath::Clamp(GridSpacing, 1.0f, 1000.0f);
    }
}
