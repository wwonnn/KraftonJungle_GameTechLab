#include "Editor/Animation/AnimationSequenceEditorDocument.h"

#include "Core/Paths.h"
#include "Editor/Animation/AnimationSequenceEditorWidget.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"
#include "Editor/EditorEngine.h"

#include "ImGui/imgui.h"

FAnimationSequenceEditorDocument::~FAnimationSequenceEditorDocument()
{
    Widget.reset();
    PreviewController.reset();
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
    EditorState.InitializeFromSequence(
        Sequence,
        PreviewController->GetLength(),
        PreviewController->IsLooping(),
        PreviewController->GetPlayRate());
    SyncEditorState();

    Widget = std::make_unique<FAnimationSequenceEditorWidget>();
    Widget->Initialize(InEditorEngine);
    Widget->BindDocumentContext(SequencePath, Sequence, PreviewController.get(), &EditorState);
    return true;
}

void FAnimationSequenceEditorDocument::Tick(float DeltaTime)
{
    if (PreviewController)
    {
        PreviewController->Tick(DeltaTime);
    }

    SyncEditorState();
}

void FAnimationSequenceEditorDocument::RenderEmbedded(float DeltaTime)
{
    // The render pipeline ticks the active document before rendering its preview
    // viewport. We synchronize once more here so the ImGui panels reflect the
    // latest playback time even when the document has no active preview scene.
    SyncEditorState();

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
    // Transport remains widget-driven for now. Adding global editor shortcuts
    // here would require broader command-id/input work outside this refactor.
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

void FAnimationSequenceEditorDocument::SyncEditorState()
{
    if (!PreviewController)
    {
        return;
    }

    // PreviewController owns runtime playback authority. EditorState mirrors
    // that data for timeline rendering and UI-only interactions.
    EditorState.SequenceLength = PreviewController->GetLength();
    EditorState.DisplayFrameRate = PreviewController->GetFrameRate();
    EditorState.TotalFrames = PreviewController->GetFrameCount();
    EditorState.bLoop = PreviewController->IsLooping();
    EditorState.PlayRate = PreviewController->GetPlayRate();
    EditorState.SetCurrentTime(PreviewController->GetCurrentTime(), false);
    EditorState.SetVisibleRange(EditorState.VisibleTimeStart, EditorState.VisibleTimeEnd);
}
