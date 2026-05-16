#include "Editor/Animation/AnimationSequenceEditorDocument.h"

#include "Core/Paths.h"
#include "Editor/Animation/AnimationSequenceEditorWidget.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"
#include "Editor/EditorEngine.h"

#include "ImGui/imgui.h"

FAnimationSequenceEditorDocument::~FAnimationSequenceEditorDocument() = default;

bool FAnimationSequenceEditorDocument::Initialize(UEditorEngine* InEditorEngine, const FString& InSequencePath, UAnimSequence* InSequence)
{
    if (!InEditorEngine || !InSequence)
    {
        return false;
    }

    EditorEngine = InEditorEngine;
    SequencePath = FPaths::Normalize(InSequencePath);
    Sequence = InSequence;
    TabId = MakeAnimationSequenceTabId(SequencePath);
    TabLabel = MakeAnimationSequenceTabLabel(SequencePath);

    PreviewController = std::make_unique<FAnimationSequencePreviewController>();
    PreviewController->SetSequence(SequencePath, Sequence);

    Widget = std::make_unique<FAnimationSequenceEditorWidget>();
    Widget->Initialize(InEditorEngine);
    Widget->BindDocumentContext(SequencePath, Sequence, PreviewController.get());
    return true;
}

void FAnimationSequenceEditorDocument::RenderEmbedded(float DeltaTime)
{
    if (Widget)
    {
        Widget->Render(DeltaTime);
    }
    else
    {
        ImGui::TextDisabled("Animation Sequence editor widget is unavailable.");
    }
}

void FAnimationSequenceEditorDocument::RenderToolbar()
{
    ImGui::TextDisabled("Animation Sequence");
    ImGui::SameLine();
    ImGui::TextUnformatted(TabLabel.c_str());
}

void FAnimationSequenceEditorDocument::BuildCommandList(FEditorCommandList& OutCommands)
{
    (void)OutCommands;
}
