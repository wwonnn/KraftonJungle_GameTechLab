#include "Editor/Animation/AnimationSequenceEditorDocument.h"

#include "Core/ResourceManager.h"
#include "Core/Paths.h"
#include "Editor/Animation/AnimationSequenceEditorWidget.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"
#include "Editor/EditorEngine.h"

#include "ImGui/imgui.h"

FAnimationSequenceEditorDocument::~FAnimationSequenceEditorDocument()
{
    Widget.reset();
    PreviewController.reset();

    if (!SequencePath.empty())
    {
        FResourceManager::Get().UnloadAnimSequence(SequencePath);
    }

    Sequence = nullptr;
}

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
    PreviewController->Initialize(InEditorEngine, SequencePath, Sequence);

    Widget = std::make_unique<FAnimationSequenceEditorWidget>();
    Widget->Initialize(InEditorEngine);
    Widget->BindDocumentContext(SequencePath, Sequence, PreviewController.get());
    return true;
}

void FAnimationSequenceEditorDocument::Tick(float DeltaTime)
{
    if (PreviewController)
    {
        PreviewController->Tick(DeltaTime);
    }
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

FSceneViewport* FAnimationSequenceEditorDocument::GetSceneViewport()
{
    return PreviewController ? PreviewController->GetSceneViewport() : nullptr;
}

const FSceneViewport* FAnimationSequenceEditorDocument::GetSceneViewport() const
{
    return PreviewController ? PreviewController->GetSceneViewport() : nullptr;
}
