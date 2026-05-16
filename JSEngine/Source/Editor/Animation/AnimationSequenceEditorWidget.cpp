#include "Editor/Animation/AnimationSequenceEditorWidget.h"

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"

#include "ImGui/imgui.h"

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
    ImGui::TextWrapped(
        "%s",
        PreviewController ? PreviewController->GetPreviewStatusText().c_str() : "Preview controller is unavailable.");

    ImGui::Spacing();
    ImGui::TextDisabled("Timeline");
    ImGui::TextWrapped(
        "%s",
        PreviewController ? PreviewController->GetTimelineStatusText().c_str() : "Timeline is unavailable.");
}
