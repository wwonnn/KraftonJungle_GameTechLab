#include "Editor/Animation/AnimationSequenceEditorDocument.h"

#include "Animation/AnimNotifySemanticFieldNames.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Core/Paths.h"
#include "Core/ResourceManager.h"
#include "Editor/Animation/AnimationSequenceEditorWidget.h"
#include "Editor/Animation/AnimationSequenceNotifyValidation.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"
#include "Editor/Animation/AnimationSequenceViewerUtils.h"
#include "Editor/EditorEngine.h"

#include "ImGui/imgui.h"

#include <algorithm>

namespace
{
    FAnimNotifyTrack MakeDefaultNotifyTrack(int32 TrackIndex)
    {
        FAnimNotifyTrack Track;
        Track.TrackName = FName(("Track " + std::to_string(TrackIndex + 1)).c_str());
        return Track;
    }

    int32 FindNotifyEventIndexById(const FAnimNotifyTrack& Track, const FGuid& StableId)
    {
        if (!StableId.IsValid())
        {
            return -1;
        }

        for (int32 Index = 0; Index < static_cast<int32>(Track.Events.size()); ++Index)
        {
            if (Track.Events[Index].StableId == StableId)
            {
                return Index;
            }
        }

        return -1;
    }

    void SortNotifyTrackEvents(FAnimNotifyTrack& Track)
    {
        std::stable_sort(
            Track.Events.begin(),
            Track.Events.end(),
            [](const FAnimNotifyEvent& Left, const FAnimNotifyEvent& Right)
            {
                if (Left.Time == Right.Time)
                {
                    return Left.Name.ToString() < Right.Name.ToString();
                }

                return Left.Time < Right.Time;
            });
    }
}

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
    bDirty = false;
    LastNotifyValidationStatusText.clear();

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
    Widget->BindDocumentContext(this, SequencePath, Sequence, PreviewController.get(), &EditorState);
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
    ImGui::Text("%s%s", TabLabel.c_str(), bDirty ? " *" : "");
}

void FAnimationSequenceEditorDocument::BuildCommandList(FEditorCommandList& OutCommands)
{
    OutCommands.Clear();

    OutCommands.MapAction(
        EEditorCommandId::Save,
        { static_cast<int32>(ImGuiKey_S), true, false, false },
        [this]()
        {
            Save();
        },
        [this]()
        {
            return CanSave();
        });

    OutCommands.MapAction(
        EEditorCommandId::DeleteSelection,
        { static_cast<int32>(ImGuiKey_Delete), false, false, false },
        [this]()
        {
            DeleteSelectedNotify();
        },
        [this]()
        {
            return GetSelectedNotify() != nullptr;
        });
}

FSceneViewport* FAnimationSequenceEditorDocument::GetSceneViewport()
{
    return PreviewController ? PreviewController->GetSceneViewport() : nullptr;
}

const FSceneViewport* FAnimationSequenceEditorDocument::GetSceneViewport() const
{
    return PreviewController ? PreviewController->GetSceneViewport() : nullptr;
}

bool FAnimationSequenceEditorDocument::CanSave() const
{
    return AnimationSequenceViewer::IsLiveObject(Sequence) && bDirty;
}

bool FAnimationSequenceEditorDocument::Save()
{
    if (!AnimationSequenceViewer::IsLiveObject(Sequence))
    {
        LastNotifyValidationStatusText = "Save failed: animation sequence is unavailable.";
        return false;
    }

    const FAnimNotifyValidationReport ValidationReport = BuildDocumentNotifyValidationReport();
    if (!FResourceManager::Get().SaveAnimSequence(SequencePath, Sequence))
    {
        LastNotifyValidationStatusText = "Save failed: resource manager rejected the sequence write.";
        return false;
    }

    bDirty = false;
    if (ValidationReport.HasAnyIssues())
    {
        LastNotifyValidationStatusText = "Saved with " + FormatAnimNotifyValidationSummary(ValidationReport) + ".";
    }
    else
    {
        LastNotifyValidationStatusText = "Saved with no notify validation issues.";
    }
    return true;
}

FAnimNotifyValidationReport FAnimationSequenceEditorDocument::BuildSelectedNotifyValidationReport() const
{
    const FAnimNotifyEvent* SelectedNotify = GetSelectedNotify();
    if (!SelectedNotify)
    {
        return FAnimNotifyValidationReport();
    }

    FAnimNotifyValidationContext Context;
    Context.PreviewController = PreviewController.get();
    FAnimNotifyValidationReport Report = ValidateAnimNotifyEvent(
        *SelectedNotify,
        EditorState.SelectedNotifyTrackIndex,
        EditorState.SelectedNotifyEventIndex,
        Context);

    const FAnimNotifyValidationReport DocumentReport = ValidateAnimNotifyDocument(Sequence, Context);
    for (const FAnimNotifyValidationIssue& Issue : DocumentReport.Issues)
    {
        if (Issue.Field == EAnimNotifyValidationField::General &&
            Issue.StableId == SelectedNotify->StableId &&
            Issue.TrackIndex == EditorState.SelectedNotifyTrackIndex &&
            Issue.EventIndex == EditorState.SelectedNotifyEventIndex)
        {
            Report.AddIssue(Issue);
        }
    }

    return Report;
}

FAnimNotifyValidationReport FAnimationSequenceEditorDocument::BuildDocumentNotifyValidationReport() const
{
    FAnimNotifyValidationContext Context;
    Context.PreviewController = PreviewController.get();
    return ValidateAnimNotifyDocument(Sequence, Context);
}

int32 FAnimationSequenceEditorDocument::GetNotifyTrackCount() const
{
    const TArray<FAnimNotifyTrack>* Tracks = AnimationSequenceViewer::GetSequenceNotifyTracks(Sequence);
    return Tracks ? static_cast<int32>(Tracks->size()) : 0;
}

const FAnimNotifyTrack* FAnimationSequenceEditorDocument::GetNotifyTrack(int32 TrackIndex) const
{
    const TArray<FAnimNotifyTrack>* Tracks = AnimationSequenceViewer::GetSequenceNotifyTracks(Sequence);
    if (!Tracks || TrackIndex < 0 || TrackIndex >= static_cast<int32>(Tracks->size()))
    {
        return nullptr;
    }

    return &(*Tracks)[TrackIndex];
}

FAnimNotifyTrack* FAnimationSequenceEditorDocument::GetNotifyTrack(int32 TrackIndex)
{
    TArray<FAnimNotifyTrack>* Tracks = AnimationSequenceViewer::GetSequenceNotifyTracks(Sequence);
    if (!Tracks || TrackIndex < 0 || TrackIndex >= static_cast<int32>(Tracks->size()))
    {
        return nullptr;
    }

    return &(*Tracks)[TrackIndex];
}

const FAnimNotifyEvent* FAnimationSequenceEditorDocument::GetSelectedNotify() const
{
    const_cast<FAnimationSequenceEditorDocument*>(this)->ValidateNotifySelection();

    const FAnimNotifyTrack* Track = GetNotifyTrack(EditorState.SelectedNotifyTrackIndex);
    if (!Track)
    {
        return nullptr;
    }

    if (EditorState.SelectedNotifyEventIndex < 0 ||
        EditorState.SelectedNotifyEventIndex >= static_cast<int32>(Track->Events.size()))
    {
        return nullptr;
    }

    return &Track->Events[EditorState.SelectedNotifyEventIndex];
}

FAnimNotifyEvent* FAnimationSequenceEditorDocument::GetSelectedNotify()
{
    ValidateNotifySelection();

    FAnimNotifyTrack* Track = GetNotifyTrack(EditorState.SelectedNotifyTrackIndex);
    if (!Track)
    {
        return nullptr;
    }

    if (EditorState.SelectedNotifyEventIndex < 0 ||
        EditorState.SelectedNotifyEventIndex >= static_cast<int32>(Track->Events.size()))
    {
        return nullptr;
    }

    return &Track->Events[EditorState.SelectedNotifyEventIndex];
}

bool FAnimationSequenceEditorDocument::AddNotifyAtTime(int32 TrackIndex, float TimeSeconds)
{
    TArray<FAnimNotifyTrack>* Tracks = AnimationSequenceViewer::GetSequenceNotifyTracks(Sequence);
    if (!Tracks || TrackIndex < 0)
    {
        return false;
    }

    while (TrackIndex >= static_cast<int32>(Tracks->size()))
    {
        Tracks->push_back(MakeDefaultNotifyTrack(static_cast<int32>(Tracks->size())));
    }

    FAnimNotifyTrack& Track = (*Tracks)[TrackIndex];
    if (!Track.TrackName.IsValid())
    {
        Track.TrackName = MakeDefaultNotifyTrack(TrackIndex).TrackName;
    }

    FAnimNotifyEvent NotifyEvent;
    NotifyEvent.StableId = FGuid::NewGuid();
    NotifyEvent.Name = FName("Notify");
    NotifyEvent.Time = EditorState.ClampOrSnapTime(TimeSeconds);
    NotifyEvent.Duration = 0.0f;
    NotifyEvent.Color = FColor(0.9490196f, 0.61960787f, 0.23921569f, 1.0f);
    NotifyEvent.EventType = EAnimNotifyEventType::Notify;
    NotifyEvent.NotifyClassName = GetDefaultAnimNotifyClassName(NotifyEvent.EventType);

    Track.Events.push_back(NotifyEvent);
    SortNotifyTrackEvents(Track);

    const int32 NewEventIndex = FindNotifyEventIndexById(Track, NotifyEvent.StableId);
    SelectNotify(TrackIndex, NewEventIndex);
    MarkDirty();
    return NewEventIndex >= 0;
}

bool FAnimationSequenceEditorDocument::DeleteSelectedNotify()
{
    ValidateNotifySelection();

    FAnimNotifyTrack* Track = GetNotifyTrack(EditorState.SelectedNotifyTrackIndex);
    if (!Track)
    {
        return false;
    }

    const int32 EventIndex = EditorState.SelectedNotifyEventIndex;
    if (EventIndex < 0 || EventIndex >= static_cast<int32>(Track->Events.size()))
    {
        return false;
    }

    Track->Events.erase(Track->Events.begin() + EventIndex);
    if (Track->Events.empty())
    {
        ClearNotifySelection();
    }
    else
    {
        SelectNotify(EditorState.SelectedNotifyTrackIndex, std::min(EventIndex, static_cast<int32>(Track->Events.size()) - 1));
    }

    MarkDirty();
    return true;
}

bool FAnimationSequenceEditorDocument::SetSelectedNotifyTime(float TimeSeconds, bool bApplySnap)
{
    FAnimNotifyTrack* Track = GetNotifyTrack(EditorState.SelectedNotifyTrackIndex);
    FAnimNotifyEvent* NotifyEvent = GetSelectedNotify();
    if (!Track || !NotifyEvent)
    {
        return false;
    }

    const FGuid StableId = NotifyEvent->StableId;
    NotifyEvent->Time = bApplySnap ? EditorState.ClampOrSnapTime(TimeSeconds) : EditorState.ClampTime(TimeSeconds);
    SortNotifyTrackEvents(*Track);
    const int32 NewEventIndex = FindNotifyEventIndexById(*Track, StableId);
    SelectNotify(EditorState.SelectedNotifyTrackIndex, NewEventIndex);
    MarkDirty();
    return NewEventIndex >= 0;
}

bool FAnimationSequenceEditorDocument::SetSelectedNotifyDuration(float DurationSeconds)
{
    FAnimNotifyEvent* NotifyEvent = GetSelectedNotify();
    if (!NotifyEvent)
    {
        return false;
    }

    NotifyEvent->Duration = std::max(0.0f, DurationSeconds);
    MarkDirty();
    return true;
}

bool FAnimationSequenceEditorDocument::SetSelectedNotifyName(const FName& Name)
{
    FAnimNotifyEvent* NotifyEvent = GetSelectedNotify();
    if (!NotifyEvent)
    {
        return false;
    }

    NotifyEvent->Name = Name;
    MarkDirty();
    return true;
}

bool FAnimationSequenceEditorDocument::SetSelectedNotifyColor(const FColor& Color)
{
    FAnimNotifyEvent* NotifyEvent = GetSelectedNotify();
    if (!NotifyEvent)
    {
        return false;
    }

    NotifyEvent->Color = Color;
    MarkDirty();
    return true;
}

bool FAnimationSequenceEditorDocument::SetSelectedNotifyType(EAnimNotifyEventType EventType)
{
    FAnimNotifyEvent* NotifyEvent = GetSelectedNotify();
    if (!NotifyEvent)
    {
        return false;
    }

    NotifyEvent->EventType = EventType;
    const FString DefaultClassName = GetDefaultAnimNotifyClassName(EventType);
    if (NotifyEvent->NotifyClassName.empty() ||
        NotifyEvent->NotifyClassName == "UAnimNotify" ||
        NotifyEvent->NotifyClassName == "UAnimNotifyState")
    {
        NotifyEvent->NotifyClassName = DefaultClassName;
    }

    MarkDirty();
    return true;
}

bool FAnimationSequenceEditorDocument::SetSelectedNotifyClassName(const FString& NotifyClassName)
{
    FAnimNotifyEvent* NotifyEvent = GetSelectedNotify();
    if (!NotifyEvent)
    {
        return false;
    }

    NotifyEvent->NotifyClassName = NotifyClassName.empty()
        ? GetDefaultAnimNotifyClassName(NotifyEvent->EventType)
        : NotifyClassName;
    MarkDirty();
    return true;
}

bool FAnimationSequenceEditorDocument::SetSelectedNotifyPayload(const FString& Payload)
{
    FAnimNotifyEvent* NotifyEvent = GetSelectedNotify();
    if (!NotifyEvent)
    {
        return false;
    }

    NotifyEvent->Payload = Payload;
    MarkDirty();
    return true;
}

FAnimNotifyPayloadParser FAnimationSequenceEditorDocument::GetSelectedNotifyPayloadParser() const
{
    const FAnimNotifyEvent* NotifyEvent = GetSelectedNotify();
    return NotifyEvent ? FAnimNotifyPayloadParser(NotifyEvent->Payload) : FAnimNotifyPayloadParser();
}

bool FAnimationSequenceEditorDocument::SetSelectedNotifyPayloadStringValue(const FString& Key, const FString& Value)
{
    FAnimNotifyPayloadParser Payload = GetSelectedNotifyPayloadParser();
    Payload.RemoveAny(AnimNotifySemanticFieldNames::GetLegacyAliases(Key));
    Payload.SetString(Key, Value);
    return SetSelectedNotifyPayload(Payload.SerializeCanonical());
}

bool FAnimationSequenceEditorDocument::SetSelectedNotifyPayloadNameValue(const FString& Key, const FName& Value)
{
    FAnimNotifyPayloadParser Payload = GetSelectedNotifyPayloadParser();
    Payload.RemoveAny(AnimNotifySemanticFieldNames::GetLegacyAliases(Key));
    Payload.SetName(Key, Value);
    return SetSelectedNotifyPayload(Payload.SerializeCanonical());
}

bool FAnimationSequenceEditorDocument::SetSelectedNotifyPayloadFloatValue(const FString& Key, float Value)
{
    FAnimNotifyPayloadParser Payload = GetSelectedNotifyPayloadParser();
    Payload.RemoveAny(AnimNotifySemanticFieldNames::GetLegacyAliases(Key));
    Payload.SetFloat(Key, Value);
    return SetSelectedNotifyPayload(Payload.SerializeCanonical());
}

bool FAnimationSequenceEditorDocument::SetSelectedNotifyPayloadBoolValue(const FString& Key, bool Value)
{
    FAnimNotifyPayloadParser Payload = GetSelectedNotifyPayloadParser();
    Payload.RemoveAny(AnimNotifySemanticFieldNames::GetLegacyAliases(Key));
    Payload.SetBool(Key, Value);
    return SetSelectedNotifyPayload(Payload.SerializeCanonical());
}

bool FAnimationSequenceEditorDocument::ClearSelectedNotifyPayloadValue(const FString& Key)
{
    FAnimNotifyPayloadParser Payload = GetSelectedNotifyPayloadParser();
    Payload.RemoveAny(AnimNotifySemanticFieldNames::GetLegacyAliases(Key));
    Payload.RemoveValue(Key);
    return SetSelectedNotifyPayload(Payload.SerializeCanonical());
}

void FAnimationSequenceEditorDocument::SelectNotify(int32 TrackIndex, int32 EventIndex)
{
    EditorState.SelectedCurveIndex = -1;
    EditorState.HoveredCurveIndex = -1;
    EditorState.SelectedNotifyTrackIndex = TrackIndex;
    EditorState.SelectedNotifyEventIndex = EventIndex;
    EditorState.HoveredNotifyTrackIndex = TrackIndex;
    EditorState.HoveredNotifyEventIndex = EventIndex;
}

void FAnimationSequenceEditorDocument::ClearNotifySelection()
{
    EditorState.SelectedNotifyTrackIndex = -1;
    EditorState.SelectedNotifyEventIndex = -1;
    EditorState.HoveredNotifyTrackIndex = -1;
    EditorState.HoveredNotifyEventIndex = -1;
    EditorState.DraggedNotifyTrackIndex = -1;
    EditorState.DraggedNotifyEventIndex = -1;
    EditorState.bDraggingNotify = false;
}

void FAnimationSequenceEditorDocument::MarkDirty()
{
    bDirty = true;
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

    const UAnimDataModel* DataModel = AnimationSequenceViewer::GetValidAnimDataModel(Sequence);
    const int32 CurveCount = DataModel ? static_cast<int32>(DataModel->CurveData.FloatCurves.size()) : 0;
    if (EditorState.SelectedCurveIndex >= CurveCount)
    {
        EditorState.SelectedCurveIndex = -1;
    }
    if (EditorState.HoveredCurveIndex >= CurveCount)
    {
        EditorState.HoveredCurveIndex = -1;
    }

    ValidateNotifySelection();
}

void FAnimationSequenceEditorDocument::ValidateNotifySelection()
{
    const TArray<FAnimNotifyTrack>* Tracks = AnimationSequenceViewer::GetSequenceNotifyTracks(Sequence);
    if (!Tracks || Tracks->empty())
    {
        ClearNotifySelection();
        return;
    }

    if (EditorState.SelectedNotifyTrackIndex < 0 ||
        EditorState.SelectedNotifyTrackIndex >= static_cast<int32>(Tracks->size()))
    {
        ClearNotifySelection();
        return;
    }

    const FAnimNotifyTrack& Track = (*Tracks)[EditorState.SelectedNotifyTrackIndex];
    if (EditorState.SelectedNotifyEventIndex < 0 ||
        EditorState.SelectedNotifyEventIndex >= static_cast<int32>(Track.Events.size()))
    {
        EditorState.SelectedNotifyEventIndex = -1;
    }

    if (EditorState.HoveredNotifyTrackIndex >= static_cast<int32>(Tracks->size()))
    {
        EditorState.HoveredNotifyTrackIndex = -1;
        EditorState.HoveredNotifyEventIndex = -1;
    }

    if (EditorState.DraggedNotifyTrackIndex >= static_cast<int32>(Tracks->size()))
    {
        EditorState.DraggedNotifyTrackIndex = -1;
        EditorState.DraggedNotifyEventIndex = -1;
        EditorState.bDraggingNotify = false;
    }
}
