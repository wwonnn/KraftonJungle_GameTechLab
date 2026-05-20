#include "Editor/Animation/AnimationSequenceEditorWidget.h"

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Asset/SkeletalMesh.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"
#include "Editor/Animation/AnimationSequenceViewerUtils.h"
#include "Editor/EditorEngine.h"
#include "Editor/UI/EditorMainPanelViewportToolbarHelpers.h"
#include "Editor/Viewport/SkeletalMeshViewportClient.h"
#include "Editor/Viewport/EditorViewportClient.h"
#include "Editor/Viewport/FSceneViewport.h"
#include "Editor/Viewport/ViewportLayout.h"
#include "Engine/Runtime/WindowsWindow.h"
#include "Render/Common/RenderTypes.h"

#include "ImGui/imgui.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>

namespace
{
    constexpr float PreviewSkeletonSplitterWidth = 4.0f;
    constexpr float PreviewOverlayMargin = 10.0f;
    constexpr float PreviewOverlayPadding = 8.0f;
    constexpr float PreviewOverlayLineSpacing = 2.0f;
    constexpr float PreviewToolbarPaddingX = 8.0f;

    bool UsesAbsoluteImGuiCoordinates()
    {
        return (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
    }

    POINT ImGuiScreenToClientPoint(FWindowsWindow* Window, const ImVec2& Point)
    {
        POINT ClientPoint = { static_cast<LONG>(Point.x), static_cast<LONG>(Point.y) };
        if (Window && Window->GetHWND() && UsesAbsoluteImGuiCoordinates())
        {
            ::ScreenToClient(Window->GetHWND(), &ClientPoint);
        }

        return ClientPoint;
    }

    void SetOpaqueBlendStateCallback(const ImDrawList*, const ImDrawCmd* Cmd)
    {
        ID3D11DeviceContext* DeviceContext = static_cast<ID3D11DeviceContext*>(Cmd->UserCallbackData);
        if (!DeviceContext)
        {
            return;
        }

        const float BlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
        DeviceContext->OMSetBlendState(nullptr, BlendFactor, 0xffffffff);
    }

    float GetPreviewOverlayHeight(const TArray<FString>& Lines)
    {
        if (Lines.empty())
        {
            return 0.0f;
        }

        const float LineHeight = ImGui::GetTextLineHeight();
        return
            PreviewOverlayPadding * 2.0f +
            LineHeight * static_cast<float>(Lines.size()) +
            PreviewOverlayLineSpacing * static_cast<float>(std::max<int32>(0, static_cast<int32>(Lines.size()) - 1));
    }

    void DrawPreviewOverlay(const ImVec2& Min, const ImVec2& Max, const TArray<FString>& Lines)
    {
        if (Lines.empty())
        {
            return;
        }

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        const float AvailableWidth = std::max(1.0f, Max.x - Min.x - PreviewOverlayMargin * 2.0f);
        float MaxTextWidth = 0.0f;
        for (const FString& Line : Lines)
        {
            MaxTextWidth = std::max(MaxTextWidth, ImGui::CalcTextSize(Line.c_str()).x);
        }

        const float OverlayWidth = std::min(AvailableWidth, MaxTextWidth + PreviewOverlayPadding * 2.0f);
        const float OverlayHeight = GetPreviewOverlayHeight(Lines);
        const ImVec2 OverlayMin(Min.x + PreviewOverlayMargin, Min.y + PreviewOverlayMargin + 30.0f);
        const ImVec2 OverlayMax(OverlayMin.x + OverlayWidth, OverlayMin.y + OverlayHeight);

        DrawList->AddRectFilled(OverlayMin, OverlayMax, IM_COL32(16, 19, 25, 210), 6.0f);
        DrawList->AddRect(OverlayMin, OverlayMax, IM_COL32(255, 255, 255, 28), 6.0f);

        float TextY = OverlayMin.y + PreviewOverlayPadding;
        for (const FString& Line : Lines)
        {
            DrawList->AddText(
                ImVec2(OverlayMin.x + PreviewOverlayPadding, TextY),
                IM_COL32(230, 234, 242, 255),
                Line.c_str());
            TextY += ImGui::GetTextLineHeight() + PreviewOverlayLineSpacing;
        }
    }
}

TArray<FString> FAnimationSequenceEditorWidget::BuildPreviewOverlayLines() const
{
    TArray<FString> PreviewOverlayLines;
    PreviewOverlayLines.push_back(
        AnimationSequenceViewer::IsLiveObject(Sequence) ? Sequence->GetName() : FString("Animation Sequence"));

    const UAnimDataModel* DataModel = AnimationSequenceViewer::GetValidAnimDataModel(Sequence);
    if (DataModel)
    {
        PreviewOverlayLines.push_back(
            std::to_string(DataModel->NumberOfFrames) +
            " frames  |  " +
            std::to_string(DataModel->NumberOfKeys) +
            " keys  |  " +
            std::to_string(static_cast<int32>(DataModel->BoneAnimationTracks.size())) +
            " bone tracks");
        PreviewOverlayLines.push_back(
            std::to_string(static_cast<int32>(DataModel->CurveData.FloatCurves.size())) +
            " curves  |  " +
            std::to_string(AnimationSequenceViewer::GetNotifyEventCount(Sequence)) +
            " notifies");
        PreviewOverlayLines.push_back(
            std::to_string(DataModel->FrameRate.Numerator) +
            " / " +
            std::to_string(DataModel->FrameRate.Denominator) +
            " fps");
    }
    else
    {
        PreviewOverlayLines.push_back("Sequence data model is unavailable.");
    }

    return PreviewOverlayLines;
}

void FAnimationSequenceEditorWidget::RenderPreviewPane(
    float PreviewHeight,
    const TArray<FString>& PreviewOverlayLines)
{
    bool bBlockViewportInputThisFrame = bSuppressEmbeddedPreviewViewportInput;
    ImVec2 PreviewSize(ImGui::GetContentRegionAvail().x, PreviewHeight);
    PreviewSize.x = std::max(PreviewSize.x, 1.0f);
    PreviewSize.y = std::max(PreviewSize.y, 1.0f);

    ImGui::BeginChild("AnimSequencePreviewViewport", PreviewSize, true, ImGuiWindowFlags_NoScrollbar);

    const USkeletalMesh* PreviewMesh = PreviewController ? PreviewController->GetPreviewMesh() : nullptr;
    const FSkeletalMesh* PreviewMeshData = PreviewMesh ? PreviewMesh->GetMeshData() : nullptr;
    if (PreviewMeshData)
    {
        PreviewSkeletonTreeWidth = std::clamp(
            PreviewSkeletonTreeWidth,
            180.0f,
            std::max(180.0f, PreviewSize.x * 0.4f));

        const float ViewportWidth = std::max(
            PreviewSize.x - PreviewSkeletonTreeWidth - PreviewSkeletonSplitterWidth,
            160.0f);
        const float ViewportToolbarHeight = static_cast<float>(FEditorViewportLayout::DefaultViewportToolbarHeight);
        const float ViewportContentHeight = std::max(PreviewSize.y - ViewportToolbarHeight, 80.0f);

        RenderPreviewSkeletonTree(PreviewSize.y);

        ImGui::SameLine(0.0f, 0.0f);
        ImGui::InvisibleButton(
            "##AnimSequencePreviewSkeletonSplitter",
            ImVec2(PreviewSkeletonSplitterWidth, PreviewSize.y));
        if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImGui::GetItemRectMin(),
            ImGui::GetItemRectMax(),
            ImGui::GetColorU32(
                ImGui::IsItemActive()
                    ? ImGuiCol_SeparatorActive
                    : (ImGui::IsItemHovered() ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator)),
            2.0f);
        if (ImGui::IsItemActive())
        {
            bBlockViewportInputThisFrame = true;
            PreviewSkeletonTreeWidth = std::clamp(
                PreviewSkeletonTreeWidth + ImGui::GetIO().MouseDelta.x,
                180.0f,
                std::max(180.0f, PreviewSize.x * 0.4f));
        }

        ImGui::SameLine(0.0f, 0.0f);
        ImGui::BeginChild("##AnimSequencePreviewViewportPane", ImVec2(ViewportWidth, PreviewSize.y), false, ImGuiWindowFlags_NoScrollbar);
        RenderPreviewViewportToolbar();

        ImGui::BeginChild(
            "##AnimSequencePreviewViewportContent",
            ImVec2(0.0f, ViewportContentHeight),
            false,
            ImGuiWindowFlags_NoScrollbar);
        if (PreviewController)
        {
            PreviewController->SetViewportSize(static_cast<int32>(ViewportWidth), static_cast<int32>(ViewportContentHeight));
        }

        if (PreviewController && PreviewController->HasValidPreview() && PreviewController->GetPreviewSRV())
        {
            RenderPreviewImage(
                ImVec2(ViewportWidth, ViewportContentHeight),
                PreviewOverlayLines,
                bBlockViewportInputThisFrame);
        }
        else
        {
            RenderPreviewFallback(ImVec2(ViewportWidth, ViewportContentHeight), PreviewOverlayLines);
        }
        ImGui::EndChild();
        ImGui::EndChild();
    }
    else
    {
        PreviewSkeletonTreeCachedMesh = nullptr;
        PreviewSkeletonTreeChildren.clear();
        PreviewSkeletonTreeSelectedBoneIndex = -1;
        PendingPreviewBoneTreeOpenStateRoot = -1;
        if (PreviewController)
        {
            PreviewController->ClearPreviewSelection();
        }
        if (PreviewController)
        {
            PreviewController->SetViewportSize(static_cast<int32>(PreviewSize.x), static_cast<int32>(PreviewSize.y));
        }

        if (PreviewController && PreviewController->HasValidPreview() && PreviewController->GetPreviewSRV())
        {
            RenderPreviewImage(PreviewSize, PreviewOverlayLines, bBlockViewportInputThisFrame);
        }
        else
        {
            RenderPreviewFallback(PreviewSize, PreviewOverlayLines);
        }
    }

    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderPreviewViewportToolbar()
{
    constexpr ImGuiWindowFlags ToolbarFlags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;
    constexpr float ToolbarHeight = static_cast<float>(FEditorViewportLayout::DefaultViewportToolbarHeight);

    ImGui::BeginChild("##AnimSequencePreviewViewportToolbar", ImVec2(0.0f, ToolbarHeight), false, ToolbarFlags);
    ImGui::SetCursorPosY(std::max(0.0f, (ToolbarHeight - ImGui::GetFrameHeight()) * 0.5f));
    ImGui::SetCursorPosX(PreviewToolbarPaddingX);

    FSceneViewport* SceneViewport = PreviewController ? PreviewController->GetSceneViewport() : nullptr;
    FEditorViewportClient* Client = SceneViewport ? SceneViewport->GetClient() : nullptr;
    FSkeletalMeshViewportClient* SkeletalViewportClient = Client ? static_cast<FSkeletalMeshViewportClient*>(Client) : nullptr;

    if (!Client)
    {
        ImGui::TextDisabled("Viewport controls are available after preview initialization.");
        ImGui::EndChild();
        return;
    }

    char ViewportTypeLabel[64] = {};
    snprintf(
        ViewportTypeLabel,
        sizeof(ViewportTypeLabel),
        "%s v",
        FEditorMainPanelViewportToolbarHelpers::GetViewportTypeName(Client->GetViewportType()));
    if (ImGui::Button(ViewportTypeLabel))
    {
        ImGui::OpenPopup("##AnimSequencePreviewViewportTypePopup");
    }
    if (ImGui::BeginPopup("##AnimSequencePreviewViewportTypePopup"))
    {
        static constexpr EEditorViewportType ViewportTypes[] = {
            EVT_Perspective,
            EVT_OrthoTop,
            EVT_OrthoBottom,
            EVT_OrthoFront,
            EVT_OrthoBack,
            EVT_OrthoLeft,
            EVT_OrthoRight,
        };
        for (EEditorViewportType Type : ViewportTypes)
        {
            if (ImGui::MenuItem(
                FEditorMainPanelViewportToolbarHelpers::GetViewportTypeName(Type),
                nullptr,
                Client->GetViewportType() == Type))
            {
                Client->SetViewportType(Type);
                Client->ApplyCameraMode();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    char CameraSpeedLabel[48] = {};
    snprintf(
        CameraSpeedLabel,
        sizeof(CameraSpeedLabel),
        "Cam %.1fx v",
        FEditorMainPanelViewportToolbarHelpers::GetCameraSpeedMultiplier(Client));
    if (ImGui::Button(CameraSpeedLabel))
    {
        ImGui::OpenPopup("##AnimSequencePreviewCameraSpeedPopup");
    }
    if (ImGui::BeginPopup("##AnimSequencePreviewCameraSpeedPopup"))
    {
        float SpeedMultiplier = FEditorMainPanelViewportToolbarHelpers::GetCameraSpeedMultiplier(Client);
        if (ImGui::SliderFloat(
            "Speed",
            &SpeedMultiplier,
            0.01f,
            FEditorMainPanelViewportToolbarHelpers::MaxCameraSpeedMultiplier,
            "%.2fx"))
        {
            FEditorMainPanelViewportToolbarHelpers::SetCameraSpeedMultiplier(Client, SpeedMultiplier);
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Show v"))
    {
        ImGui::OpenPopup("##AnimSequencePreviewShowPopup");
    }
    if (ImGui::BeginPopup("##AnimSequencePreviewShowPopup"))
    {
        if (SkeletalViewportClient)
        {
            FSkeletalViewerShowFlags& ShowFlags = SkeletalViewportClient->GetShowFlags();
            ImGui::MenuItem("Bones", nullptr, &ShowFlags.bShowBones);
        }
        else
        {
            ImGui::TextDisabled("Show flags are unavailable.");
        }
        ImGui::EndPopup();
    }

    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderPreviewSkeletonTree(float Height)
{
    ImGui::BeginChild("##AnimSequencePreviewSkeletonTree", ImVec2(PreviewSkeletonTreeWidth, Height), false);
    ImGui::TextDisabled("Skeleton Tree");
    ImGui::Separator();

    const USkeletalMesh* PreviewMesh = PreviewController ? PreviewController->GetPreviewMesh() : nullptr;
    const FSkeletalMesh* MeshData = PreviewMesh ? PreviewMesh->GetMeshData() : nullptr;
    if (!MeshData)
    {
        PreviewSkeletonTreeCachedMesh = nullptr;
        PreviewSkeletonTreeChildren.clear();
        PreviewSkeletonTreeSelectedBoneIndex = -1;
        PendingPreviewBoneTreeOpenStateRoot = -1;
        ImGui::TextDisabled("Preview mesh is unavailable.");
        ImGui::EndChild();
        return;
    }

    if (PreviewSkeletonTreeCachedMesh != MeshData)
    {
        PreviewSkeletonTreeCachedMesh = MeshData;
        PreviewSkeletonTreeSelectedBoneIndex = -1;
        if (PreviewController)
        {
            PreviewController->ClearPreviewSelection();
        }
        RebuildPreviewSkeletonTreeCaches(MeshData);
    }

    ApplyPendingPreviewBoneTreeOpenState(MeshData);
    const TArray<FBoneInfo>& Bones = MeshData->GetBones();
    for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(Bones.size()); ++BoneIndex)
    {
        if (Bones[BoneIndex].ParentIndex == -1)
        {
            DrawPreviewSkeletonBoneNode(BoneIndex, Bones, PreviewSkeletonTreeChildren);
        }
    }

    if (PreviewSkeletonTreeSelectedBoneIndex >= 0 &&
        PreviewSkeletonTreeSelectedBoneIndex < static_cast<int32>(Bones.size()))
    {
        ImGui::Separator();
        ImGui::TextDisabled("Selected Bone");
        const FString SelectedBoneName = Bones[PreviewSkeletonTreeSelectedBoneIndex].Name.ToString();
        ImGui::Text("%s", SelectedBoneName.c_str());
    }

    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderPreviewImage(
    const ImVec2& PreviewSize,
    const TArray<FString>& PreviewOverlayLines,
    bool bBlockViewportInputThisFrame)
{
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    const ImVec2 Min = ImGui::GetCursorScreenPos();
    const ImVec2 Max(Min.x + PreviewSize.x, Min.y + PreviewSize.y);

    ID3D11DeviceContext* DeviceContext =
        EditorEngine ? EditorEngine->GetRenderer().GetFD3DDevice().GetDeviceContext() : nullptr;
    DrawList->AddCallback(SetOpaqueBlendStateCallback, DeviceContext);
    DrawList->AddImage((ImTextureID)PreviewController->GetPreviewSRV(), Min, Max);
    DrawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

    ImGui::Dummy(PreviewSize);
    RenderPreviewToolbarOverlay(Min, Max);
    DrawPreviewOverlay(Min, Max, PreviewOverlayLines);

    const bool bViewportHovered = ImGui::IsItemHovered();
    const bool bViewportClicked =
        bViewportHovered &&
        (ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
            ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
            ImGui::IsMouseClicked(ImGuiMouseButton_Middle));

    SyncEmbeddedViewportRectAndFocus(Min, Max, bViewportClicked, bBlockViewportInputThisFrame);
}

void FAnimationSequenceEditorWidget::RenderPreviewFallback(
    const ImVec2& PreviewSize,
    const TArray<FString>& PreviewOverlayLines) const
{
    const ImVec2 OverlayMin = ImGui::GetCursorScreenPos();
    const ImVec2 OverlayMax(OverlayMin.x + PreviewSize.x, OverlayMin.y + PreviewSize.y);
    ImGui::Dummy(ImVec2(0.0f, GetPreviewOverlayHeight(PreviewOverlayLines) + PreviewOverlayMargin * 2.0f));
    ImGui::TextWrapped(
        "%s",
        PreviewController ? PreviewController->GetPreviewStatusText().c_str() : "Preview controller is unavailable.");
    DrawPreviewOverlay(OverlayMin, OverlayMax, PreviewOverlayLines);
}

void FAnimationSequenceEditorWidget::RenderPreviewToolbarOverlay(const ImVec2& Min, const ImVec2& Max) const
{
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    const ImVec2 BarMin(Min.x + PreviewOverlayMargin, Min.y + PreviewOverlayMargin);
    const ImVec2 BarMax(Max.x - PreviewOverlayMargin, Min.y + PreviewOverlayMargin + 22.0f);
    DrawList->AddRectFilled(BarMin, BarMax, IM_COL32(12, 15, 20, 210), 6.0f);
    DrawList->AddRect(BarMin, BarMax, IM_COL32(255, 255, 255, 24), 6.0f);

    const FString SequenceName =
        AnimationSequenceViewer::IsLiveObject(Sequence) ? Sequence->GetName() : FString("Animation Sequence");
    DrawList->AddText(ImVec2(BarMin.x + 8.0f, BarMin.y + 4.0f), IM_COL32(233, 237, 244, 255), SequenceName.c_str());

    char StatusBuffer[128] = {};
    if (PreviewController)
    {
        snprintf(
            StatusBuffer,
            sizeof(StatusBuffer),
            "%s  |  %.3fs  |  %.2fx",
            PreviewController->IsPlaying() ? "Playing" : "Paused",
            PreviewController->GetCurrentTime(),
            PreviewController->GetPlayRate());
    }
    else
    {
        snprintf(StatusBuffer, sizeof(StatusBuffer), "Preview unavailable");
    }

    const ImVec2 StatusTextSize = ImGui::CalcTextSize(StatusBuffer);
    DrawList->AddText(
        ImVec2(BarMax.x - StatusTextSize.x - 10.0f, BarMin.y + 4.0f),
        IM_COL32(176, 183, 195, 255),
        StatusBuffer);
}

void FAnimationSequenceEditorWidget::SyncEmbeddedViewportRectAndFocus(
    const ImVec2& Min,
    const ImVec2& Max,
    bool bViewportClicked,
    bool bBlockViewportInputThisFrame)
{
    if (!PreviewController)
    {
        return;
    }

    FSceneViewport* SceneViewport = PreviewController->GetSceneViewport();
    if (!SceneViewport)
    {
        return;
    }

    FViewportRect NewRect = {};
    if (bBlockViewportInputThisFrame)
    {
        SceneViewport->SetRect(NewRect);
        return;
    }

    const POINT ClientMin = ImGuiScreenToClientPoint(EditorEngine ? EditorEngine->GetWindow() : nullptr, Min);
    NewRect.X = static_cast<int32>(ClientMin.x);
    NewRect.Y = static_cast<int32>(ClientMin.y);
    NewRect.Width = static_cast<int32>(Max.x - Min.x);
    NewRect.Height = static_cast<int32>(Max.y - Min.y);

    // The scene viewport still receives renderer/input updates through the
    // editor viewport path, so the embedded ImGui image must keep its rect in
    // sync with the real viewport coordinates.
    SceneViewport->SetRect(NewRect);
    if (FEditorViewportClient* Client = SceneViewport->GetClient())
    {
        Client->SetViewportSize(static_cast<float>(NewRect.Width), static_cast<float>(NewRect.Height));
    }

    if (bViewportClicked && EditorEngine)
    {
        EditorEngine->FocusViewportInput(SceneViewport);
    }
}

void FAnimationSequenceEditorWidget::DrawPreviewSkeletonBoneNode(
    int32 BoneIndex,
    const TArray<FBoneInfo>& Bones,
    const TArray<TArray<int32>>& Children)
{
    if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(Bones.size()))
    {
        return;
    }

    const FBoneInfo& Bone = Bones[BoneIndex];
    const bool bHasChildren =
        BoneIndex >= 0 &&
        BoneIndex < static_cast<int32>(Children.size()) &&
        !Children[BoneIndex].empty();

    ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!bHasChildren)
    {
        Flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (PreviewSkeletonTreeSelectedBoneIndex == BoneIndex)
    {
        Flags |= ImGuiTreeNodeFlags_Selected;
    }

    const FString BoneName = Bone.Name.ToString();
    const bool bOpen =
        ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(BoneIndex)), Flags, "%s", BoneName.c_str());
    if (ImGui::IsItemClicked())
    {
        PreviewSkeletonTreeSelectedBoneIndex = BoneIndex;
        if (PreviewController)
        {
            PreviewController->SelectPreviewBone(BoneIndex);
        }
        if (FSceneViewport* SceneViewport = PreviewController ? PreviewController->GetSceneViewport() : nullptr)
        {
            if (FSkeletalMeshViewportClient* Client = static_cast<FSkeletalMeshViewportClient*>(SceneViewport->GetClient()))
            {
                Client->GetShowFlags().bShowBones = true;
            }
        }
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Expand Children", nullptr, false, bHasChildren))
        {
            QueuePreviewBoneSubtreeOpenState(BoneIndex, true);
        }
        if (ImGui::MenuItem("Collapse Children", nullptr, false, bHasChildren))
        {
            QueuePreviewBoneSubtreeOpenState(BoneIndex, false);
        }
        ImGui::EndPopup();
    }

    if (!bOpen)
    {
        return;
    }

    for (int32 ChildIndex : Children[BoneIndex])
    {
        DrawPreviewSkeletonBoneNode(ChildIndex, Bones, Children);
    }
    ImGui::TreePop();
}

void FAnimationSequenceEditorWidget::RebuildPreviewSkeletonTreeCaches(const FSkeletalMesh* MeshData)
{
    PreviewSkeletonTreeChildren.clear();
    if (!MeshData)
    {
        return;
    }

    const TArray<FBoneInfo>& Bones = MeshData->GetBones();
    PreviewSkeletonTreeChildren.resize(Bones.size());
    for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(Bones.size()); ++BoneIndex)
    {
        const int32 ParentIndex = Bones[BoneIndex].ParentIndex;
        if (ParentIndex >= 0 && ParentIndex < static_cast<int32>(PreviewSkeletonTreeChildren.size()))
        {
            PreviewSkeletonTreeChildren[ParentIndex].push_back(BoneIndex);
        }
    }
}

void FAnimationSequenceEditorWidget::QueuePreviewBoneSubtreeOpenState(int32 BoneIndex, bool bOpen)
{
    PendingPreviewBoneTreeOpenStateRoot = BoneIndex;
    bPendingPreviewBoneTreeOpenStateValue = bOpen;
}

void FAnimationSequenceEditorWidget::ApplyPendingPreviewBoneTreeOpenState(const FSkeletalMesh* MeshData)
{
    if (!MeshData || PendingPreviewBoneTreeOpenStateRoot < 0)
    {
        return;
    }

    SetPreviewBoneSubtreeOpenState(
        PendingPreviewBoneTreeOpenStateRoot,
        PreviewSkeletonTreeChildren,
        bPendingPreviewBoneTreeOpenStateValue);
    PendingPreviewBoneTreeOpenStateRoot = -1;
}

void FAnimationSequenceEditorWidget::SetPreviewBoneSubtreeOpenState(
    int32 BoneIndex,
    const TArray<TArray<int32>>& Children,
    bool bOpen)
{
    if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(Children.size()))
    {
        return;
    }

    ImGuiStorage* Storage = ImGui::GetStateStorage();
    if (!Storage)
    {
        return;
    }

    const void* NodePointer = reinterpret_cast<void*>(static_cast<intptr_t>(BoneIndex));
    const ImGuiID NodeId = ImGui::GetID(NodePointer);
    if (bOpen)
    {
        Storage->SetInt(NodeId, 1);
    }

    ImGui::PushID(NodePointer);
    for (int32 ChildIndex : Children[BoneIndex])
    {
        SetPreviewBoneSubtreeOpenState(ChildIndex, Children, bOpen);
    }
    ImGui::PopID();

    if (!bOpen)
    {
        Storage->SetInt(NodeId, 0);
    }
}
