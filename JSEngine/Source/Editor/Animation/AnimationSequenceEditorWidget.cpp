#include "Editor/Animation/AnimationSequenceEditorWidget.h"

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"
#include "Editor/EditorEngine.h"
#include "Render/Common/RenderTypes.h"

#include "ImGui/imgui.h"

#include <algorithm>

namespace
{
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
}

void FAnimationSequenceEditorWidget::BindDocumentContext(
    const FString& InSequencePath,
    UAnimSequence* InSequence,
    FAnimationSequencePreviewController* InPreviewController)
{
    SequencePath = InSequencePath;
    Sequence = InSequence;
    PreviewController = InPreviewController;
}

void FAnimationSequenceEditorWidget::Render(float DeltaTime)
{
    (void)DeltaTime;

    UAnimDataModel* DataModel = Sequence ? Sequence->DataModel : nullptr;

    ImGui::Spacing();
    ImGui::TextDisabled("Animation Sequence");
    ImGui::Separator();

    ImGui::Text("Asset Path: %s", SequencePath.empty() ? "(none)" : SequencePath.c_str());
    ImGui::Text("Sequence Name: %s", Sequence ? Sequence->GetName().c_str() : "(unloaded)");

    if (!DataModel)
    {
        ImGui::Spacing();
        ImGui::TextDisabled("Sequence data model is unavailable.");
        return;
    }

    ImGui::Text("Frame Rate: %d / %d", DataModel->FrameRate.Numerator, DataModel->FrameRate.Denominator);
    ImGui::Text("Number Of Frames: %d", DataModel->NumberOfFrames);
    ImGui::Text("Number Of Keys: %d", DataModel->NumberOfKeys);
    ImGui::Text("Bone Track Count: %d", static_cast<int32>(DataModel->BoneAnimationTracks.size()));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Preview");

    const bool bHasController = PreviewController != nullptr;
    const bool bHasPreview = bHasController && PreviewController->HasValidPreview();
    const bool bCanControl = bHasController && bHasPreview;
    const float PreviewHeight = std::max(240.0f, ImGui::GetContentRegionAvail().y * 0.55f);
    ImVec2 PreviewSize(ImGui::GetContentRegionAvail().x, PreviewHeight);
    PreviewSize.x = std::max(PreviewSize.x, 1.0f);
    PreviewSize.y = std::max(PreviewSize.y, 1.0f);

    ImGui::BeginChild("AnimSequencePreviewViewport", PreviewSize, true, ImGuiWindowFlags_NoScrollbar);

    if (bHasController)
    {
        PreviewController->SetViewportSize(static_cast<int32>(PreviewSize.x), static_cast<int32>(PreviewSize.y));
    }

    if (bHasPreview && PreviewController->GetPreviewSRV())
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
    }
    else
    {
        ImGui::TextWrapped(
            "%s",
            bHasController ? PreviewController->GetPreviewStatusText().c_str() : "Preview controller is unavailable.");
    }

    ImGui::EndChild();

    const float CurrentTime = bHasController ? PreviewController->GetCurrentTime() : 0.0f;
    const float SequenceLength = bHasController ? PreviewController->GetLength() : 0.0f;
    float ScrubTime = CurrentTime;

    ImGui::Spacing();
    ImGui::Text("Current Time: %.3fs / %.3fs", CurrentTime, SequenceLength);

    const bool bCanScrub = bCanControl && SequenceLength > 0.0f;
    if (!bCanScrub)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::SliderFloat("Time", &ScrubTime, 0.0f, std::max(SequenceLength, 0.001f), "%.3f s"))
    {
        PreviewController->SetCurrentTime(ScrubTime);
    }

    if (!bCanScrub)
    {
        ImGui::EndDisabled();
    }

    bool bLooping = bHasController ? PreviewController->IsLooping() : false;
    float PlayRate = bHasController ? PreviewController->GetPlayRate() : 1.0f;

    if (!bCanControl)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Play") && bHasController)
    {
        PreviewController->Play();
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause") && bHasController)
    {
        PreviewController->Pause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop") && bHasController)
    {
        PreviewController->Stop();
    }

    if (ImGui::Checkbox("Loop", &bLooping) && bHasController)
    {
        PreviewController->SetLooping(bLooping);
    }

    if (ImGui::SliderFloat("Play Rate", &PlayRate, -2.0f, 2.0f, "%.2fx") && bHasController)
    {
        PreviewController->SetPlayRate(PlayRate);
    }

    if (!bCanControl)
    {
        ImGui::EndDisabled();
    }

    ImGui::Spacing();
    ImGui::TextDisabled(
        "%s",
        bHasController ? PreviewController->GetTimelineStatusText().c_str() : "Timeline is unavailable.");
}
