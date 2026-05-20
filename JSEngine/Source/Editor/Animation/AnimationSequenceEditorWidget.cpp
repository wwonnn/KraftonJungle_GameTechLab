#include "Editor/Animation/AnimationSequenceEditorWidget.h"

#include "Animation/AnimNotify.h"
#include "Animation/AnimNotifyPayloadParser.h"
#include "Animation/AnimNotifySemanticFieldNames.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Editor/Animation/AnimationSequenceEditorDocument.h"
#include "Editor/Animation/AnimationSequenceNotifyPayloadSchema.h"
#include "Editor/Animation/AnimationSequenceNotifyValidation.h"
#include "Editor/Animation/AnimationSequenceEditorState.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"
#include "Editor/Animation/AnimationSequenceSequencerLayout.h"
#include "Editor/Animation/AnimationSequenceViewerUiHelpers.h"
#include "Editor/Animation/AnimationSequenceViewerUtils.h"
#include "Core/Paths.h"
#include "Engine/Asset/CurveFloatAsset.h"
#include "Object/ObjectFactory.h"

#include "ImGui/imgui.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

namespace
{
    constexpr float NotifyPanelSectionSpacing = 10.0f;
    constexpr float NotifyPanelSectionIndent = 6.0f;
    constexpr float NotifyPanelToolbarSpacing = 6.0f;
    constexpr float NotifyPanelFieldMinWidth = 220.0f;
    constexpr float NotifyPanelFieldMaxWidth = 440.0f;

    void SetNotifyPanelFieldWidth()
    {
        AnimationSequenceViewerUi::SetClampedWidthItem(
            0.72f,
            NotifyPanelFieldMinWidth,
            NotifyPanelFieldMaxWidth);
    }

    TArray<FString> CollectNotifyClassOptions(EAnimNotifyEventType EventType)
    {
        TArray<const UClass*> RegisteredClasses;
        FObjectFactory::Get().GetRegisteredClasses(RegisteredClasses);

        const UClass* RequiredBaseClass =
            EventType == EAnimNotifyEventType::NotifyState ? UAnimNotifyState::StaticClass() : UAnimNotify::StaticClass();
        const UClass* ExcludedBaseClass =
            EventType == EAnimNotifyEventType::NotifyState ? nullptr : UAnimNotifyState::StaticClass();

        TArray<FString> Options;
        for (const UClass* Class : RegisteredClasses)
        {
            if (!Class || !Class->GetName() || !Class->IsA(RequiredBaseClass))
            {
                continue;
            }

            if (ExcludedBaseClass && Class->IsA(ExcludedBaseClass))
            {
                continue;
            }

            Options.push_back(Class->GetName());
        }

        std::sort(Options.begin(), Options.end());
        Options.erase(std::unique(Options.begin(), Options.end()), Options.end());
        return Options;
    }

    bool ContainsNotifyClassOption(const TArray<FString>& Options, const FString& Value)
    {
        return std::find(Options.begin(), Options.end(), Value) != Options.end();
    }

    ImVec4 GetValidationSeverityColor(EAnimNotifyValidationSeverity Severity)
    {
        switch (Severity)
        {
        case EAnimNotifyValidationSeverity::Error:
            return ImVec4(0.95f, 0.35f, 0.35f, 1.0f);
        case EAnimNotifyValidationSeverity::Warning:
            return ImVec4(0.98f, 0.76f, 0.32f, 1.0f);
        case EAnimNotifyValidationSeverity::Info:
        default:
            return ImVec4(0.58f, 0.72f, 0.96f, 1.0f);
        }
    }

    const char* GetValidationSeverityLabel(EAnimNotifyValidationSeverity Severity)
    {
        switch (Severity)
        {
        case EAnimNotifyValidationSeverity::Error:
            return "Error";
        case EAnimNotifyValidationSeverity::Warning:
            return "Warning";
        case EAnimNotifyValidationSeverity::Info:
        default:
            return "Info";
        }
    }

    template <size_t BufferSize>
    void CopyStringToBuffer(const FString& Source, std::array<char, BufferSize>& Buffer)
    {
        Buffer.fill('\0');
        strncpy_s(Buffer.data(), Buffer.size(), Source.c_str(), _TRUNCATE);
    }

    template <size_t BufferSize>
    void CopyPayloadFieldToBuffer(
        const FAnimNotifyPayloadParser& Payload,
        const FAnimNotifyPayloadFieldDefinition& Field,
        std::array<char, BufferSize>& Buffer)
    {
        CopyStringToBuffer(Payload.GetStringAny(GetAnimNotifyPayloadFieldLookupKeys(Field)), Buffer);
    }

    float GetSchemaFloatValue(const FAnimNotifyPayloadParser& Payload, const FAnimNotifyPayloadFieldDefinition& Field)
    {
        const FAnimNotifyPayloadParser DefaultPayload(Field.DefaultValue.empty() ? FString() : Field.Key + "=" + Field.DefaultValue);
        return Payload.GetFloatAny(
            GetAnimNotifyPayloadFieldLookupKeys(Field),
            DefaultPayload.GetFloat(Field.Key, 0.0f));
    }

    int32 GetSchemaIntValue(const FAnimNotifyPayloadParser& Payload, const FAnimNotifyPayloadFieldDefinition& Field)
    {
        const FAnimNotifyPayloadParser DefaultPayload(Field.DefaultValue.empty() ? FString() : Field.Key + "=" + Field.DefaultValue);
        return Payload.GetIntAny(
            GetAnimNotifyPayloadFieldLookupKeys(Field),
            DefaultPayload.GetInt(Field.Key, 0));
    }

    bool GetSchemaBoolValue(const FAnimNotifyPayloadParser& Payload, const FAnimNotifyPayloadFieldDefinition& Field)
    {
        const FAnimNotifyPayloadParser DefaultPayload(Field.DefaultValue.empty() ? FString() : Field.Key + "=" + Field.DefaultValue);
        return Payload.GetBoolAny(
            GetAnimNotifyPayloadFieldLookupKeys(Field),
            DefaultPayload.GetBool(Field.Key, false));
    }

    const char* GetPickerUnavailableMessage(EAnimNotifyPayloadFieldEditorHint EditorHint)
    {
        switch (EditorHint)
        {
        case EAnimNotifyPayloadFieldEditorHint::SocketPicker:
            return "No preview sockets available. Enter a manual value.";
        case EAnimNotifyPayloadFieldEditorHint::ComponentPicker:
            return "No preview primitive components available. Enter a manual value.";
        case EAnimNotifyPayloadFieldEditorHint::Default:
        default:
            return nullptr;
        }
    }

    FString MakeRecentNotifyTrackLabel(const FAnimNotifyEvent& NotifyEvent)
    {
        if (NotifyEvent.SourceTrackName.IsValid())
        {
            return NotifyEvent.SourceTrackName.ToString();
        }

        if (NotifyEvent.SourceTrackIndex >= 0)
        {
            return FString("Track ") + std::to_string(NotifyEvent.SourceTrackIndex + 1);
        }

        return "(Unknown Track)";
    }

    FString MakeRecentNotifySourceLabel(const FAnimNotifyEvent& NotifyEvent)
    {
        if (!NotifyEvent.SourceSequencePath.empty())
        {
            const FString ProjectRelativePath = FPaths::ToProjectRelativePath(NotifyEvent.SourceSequencePath);
            const FString& PreferredPath = ProjectRelativePath.empty() ? NotifyEvent.SourceSequencePath : ProjectRelativePath;
            const std::filesystem::path SourcePath(FPaths::ToWide(PreferredPath));
            const FString FileName = FPaths::ToString(SourcePath.filename().wstring());
            return FileName.empty() ? PreferredPath : FileName;
        }

        if (!NotifyEvent.SourceSequenceName.empty())
        {
            return NotifyEvent.SourceSequenceName;
        }

        return "(Current Preview Sequence)";
    }

    int32 GetVisibleCurveCount(const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups)
    {
        int32 VisibleCount = 0;
        for (const FAnimationSequenceCurveViewGroup& Group : CurveGroups)
        {
            if (Group.bVisible)
            {
                VisibleCount += static_cast<int32>(Group.VisibleEntries.size());
            }
        }

        return VisibleCount;
    }

    const FFloatCurve* GetSelectedCurve(const UAnimSequence* Sequence, const FAnimationSequenceEditorState* State)
    {
        if (!State)
        {
            return nullptr;
        }

        const UAnimDataModel* DataModel = AnimationSequenceViewer::GetValidAnimDataModel(Sequence);
        if (!DataModel ||
            State->SelectedCurveIndex < 0 ||
            State->SelectedCurveIndex >= static_cast<int32>(DataModel->CurveData.FloatCurves.size()))
        {
            return nullptr;
        }

        return &DataModel->CurveData.FloatCurves[State->SelectedCurveIndex];
    }

    struct FSelectedNotifyEditorContext
    {
        bool bValid = false;
        FAnimNotifyEvent NotifySnapshot;
        FString NotifyClassName;
        FAnimNotifyValidationReport ValidationReport;
    };

    bool HasPreviewController(const FAnimationSequencePreviewController* PreviewController)
    {
        return PreviewController != nullptr;
    }

    bool CanInspectRecentFiredNotifies(const FAnimationSequencePreviewController* PreviewController)
    {
        return HasPreviewController(PreviewController);
    }

    FSelectedNotifyEditorContext BuildSelectedNotifyEditorContext(const FAnimationSequenceEditorDocument* Document)
    {
        FSelectedNotifyEditorContext Context;
        if (!Document || !Document->TryGetSelectedNotifySnapshot(Context.NotifySnapshot))
        {
            return Context;
        }

        Context.bValid = true;
        Context.NotifyClassName = Document->GetSelectedNotifyResolvedClassName();
        Context.ValidationReport = Document->BuildSelectedNotifyValidationReport();
        return Context;
    }

    struct FCurveValueRange
    {
        bool bValid = false;
        float MinValue = 0.0f;
        float MaxValue = 0.0f;
    };

    struct FCurveKeySummary
    {
        bool bValid = false;
        int32 KeyIndex = -1;
        float Time = 0.0f;
        float Value = 0.0f;
    };

    struct FCurveInspectionSnapshot
    {
        float CurrentValue = 0.0f;
        FCurveValueRange FullValueRange;
        FCurveValueRange VisibleValueRange;
        FCurveKeySummary FirstKey;
        FCurveKeySummary LastKey;
        FCurveKeySummary NearestKey;
    };

    struct FCurveInspectionContext
    {
        const UAnimDataModel* DataModel = nullptr;
        const FFloatCurve* SelectedCurve = nullptr;
        TArray<FAnimationSequenceCurveViewGroup> CurveGroups;
        int32 VisibleCurveCount = 0;
        bool bHasSnapshot = false;
        FCurveInspectionSnapshot Snapshot;
    };

    FCurveValueRange ComputeCurveValueRangeFromKeys(const FFloatCurve& Curve)
    {
        FCurveValueRange Range;
        if (Curve.Keys.empty())
        {
            return Range;
        }

        Range.bValid = true;
        Range.MinValue = Curve.Keys.front().Value;
        Range.MaxValue = Curve.Keys.front().Value;
        for (const FCurveKey& Key : Curve.Keys)
        {
            Range.MinValue = std::min(Range.MinValue, Key.Value);
            Range.MaxValue = std::max(Range.MaxValue, Key.Value);
        }
        return Range;
    }

    FCurveValueRange ComputeCurveValueRangeFromSampling(const FFloatCurve& Curve, float StartTime, float EndTime)
    {
        FCurveValueRange Range;
        constexpr int32 SampleCount = 64;
        const float SafeStart = std::min(StartTime, EndTime);
        const float SafeEnd = std::max(StartTime, EndTime);
        float MinValue = std::numeric_limits<float>::max();
        float MaxValue = -std::numeric_limits<float>::max();

        for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
        {
            const float Alpha = SampleCount > 1 ? static_cast<float>(SampleIndex) / static_cast<float>(SampleCount - 1) : 0.0f;
            const float Time = SafeStart + (SafeEnd - SafeStart) * Alpha;
            const float Value = Curve.Evaluate(Time);
            MinValue = std::min(MinValue, Value);
            MaxValue = std::max(MaxValue, Value);
        }

        if (MinValue == std::numeric_limits<float>::max() || MaxValue == -std::numeric_limits<float>::max())
        {
            return Range;
        }

        Range.bValid = true;
        Range.MinValue = MinValue;
        Range.MaxValue = MaxValue;
        return Range;
    }

    FCurveKeySummary MakeCurveKeySummary(const FFloatCurve& Curve, int32 KeyIndex)
    {
        FCurveKeySummary Summary;
        if (KeyIndex < 0 || KeyIndex >= static_cast<int32>(Curve.Keys.size()))
        {
            return Summary;
        }

        Summary.bValid = true;
        Summary.KeyIndex = KeyIndex;
        Summary.Time = Curve.Keys[KeyIndex].Time;
        Summary.Value = Curve.Keys[KeyIndex].Value;
        return Summary;
    }

    FCurveKeySummary FindNearestCurveKey(const FFloatCurve& Curve, float Time)
    {
        int32 BestIndex = -1;
        float BestDistance = std::numeric_limits<float>::max();
        for (int32 KeyIndex = 0; KeyIndex < static_cast<int32>(Curve.Keys.size()); ++KeyIndex)
        {
            const float Distance = std::fabs(Curve.Keys[KeyIndex].Time - Time);
            if (Distance < BestDistance)
            {
                BestDistance = Distance;
                BestIndex = KeyIndex;
            }
        }

        return MakeCurveKeySummary(Curve, BestIndex);
    }

    FCurveInspectionSnapshot BuildCurveInspectionSnapshot(
        const FFloatCurve& Curve,
        const FAnimationSequenceEditorState& State)
    {
        FCurveInspectionSnapshot Snapshot;
        Snapshot.CurrentValue = Curve.Evaluate(State.CurrentTime);
        Snapshot.FullValueRange = ComputeCurveValueRangeFromKeys(Curve);
        Snapshot.VisibleValueRange = ComputeCurveValueRangeFromSampling(Curve, State.VisibleTimeStart, State.VisibleTimeEnd);
        Snapshot.FirstKey = MakeCurveKeySummary(Curve, Curve.Keys.empty() ? -1 : 0);
        Snapshot.LastKey = MakeCurveKeySummary(Curve, Curve.Keys.empty() ? -1 : static_cast<int32>(Curve.Keys.size()) - 1);
        Snapshot.NearestKey = FindNearestCurveKey(Curve, State.CurrentTime);
        return Snapshot;
    }

    FCurveInspectionContext BuildCurveInspectionContext(
        const UAnimSequence* Sequence,
        const FAnimationSequenceEditorState* State)
    {
        FCurveInspectionContext Context;
        Context.DataModel = AnimationSequenceViewer::GetValidAnimDataModel(Sequence);
        if (!Context.DataModel || Context.DataModel->CurveData.FloatCurves.empty())
        {
            return Context;
        }

        if (!State)
        {
            return Context;
        }

        Context.SelectedCurve = GetSelectedCurve(Sequence, State);
        if (!Context.SelectedCurve)
        {
            Context.CurveGroups = AnimationSequenceCurveFilter::BuildCurveViewGroups(Sequence, *State);
            Context.VisibleCurveCount = GetVisibleCurveCount(Context.CurveGroups);
            return Context;
        }

        Context.Snapshot = BuildCurveInspectionSnapshot(*Context.SelectedCurve, *State);
        Context.bHasSnapshot = true;
        return Context;
    }
}

void FAnimationSequenceEditorWidget::BindDocumentContext(
    FAnimationSequenceEditorDocument* InDocument,
    const FString& InSequencePath,
    UAnimSequence* InSequence,
    FAnimationSequencePreviewController* InPreviewController,
    FAnimationSequenceEditorState* InEditorState)
{
    Document = InDocument;
    SequencePath = InSequencePath;
    Sequence = InSequence;
    PreviewController = InPreviewController;
    EditorState = InEditorState;
    NotifyDetailsBoundStableId.clear();
    NotifyNameEditBuffer.fill('\0');
    ResetNotifyPayloadFieldBuffers();
    NotifyPayloadEditBuffer.fill('\0');
    TrackFilterEditBuffer.fill('\0');
    TimelineRangeEditBuffer.fill('\0');
    ActiveTimelineRangeEditPopupId.clear();
    ActiveNotifyValidationSeverityFilter = ENotifyValidationBrowserSeverityFilter::All;
    PlaybackSpeedCustomValue = InPreviewController ? std::fabs(InPreviewController->GetPlayRate()) : 1.0f;
    PreviewSkeletonTreeChildren.clear();
    PreviewSkeletonTreeCachedMesh = nullptr;
    PreviewSkeletonTreeSelectedBoneIndex = -1;
    PendingPreviewBoneTreeOpenStateRoot = -1;
    bPendingPreviewBoneTreeOpenStateValue = false;
    PreviewSkeletonTreeWidth = 240.0f;
}

//----------------------------------------------------------------------------
// Top-level viewer orchestration
//----------------------------------------------------------------------------

void FAnimationSequenceEditorWidget::Render(float DeltaTime)
{
    (void)DeltaTime;

    const TArray<FString> PreviewOverlayLines = BuildPreviewOverlayLines();
    const float AvailableHeight = std::max(ImGui::GetContentRegionAvail().y, 300.0f);
    const float MinLayoutHeight =
        FAnimationSequenceSequencerLayout::PreviewMinHeight +
        FAnimationSequenceSequencerLayout::SequencerMinHeight +
        FAnimationSequenceSequencerLayout::SectionSplitterHeight;
    const float SplitLayoutHeight = std::max(AvailableHeight, MinLayoutHeight);
    const float MaxPreviewHeight =
        std::max(
            FAnimationSequenceSequencerLayout::PreviewMinHeight,
            SplitLayoutHeight -
            FAnimationSequenceSequencerLayout::SequencerMinHeight -
            FAnimationSequenceSequencerLayout::SectionSplitterHeight);

    if (EditorState && EditorState->PreviewPaneHeight <= 0.0f)
    {
        EditorState->PreviewPaneHeight = SplitLayoutHeight * 0.58f;
    }

    const float PreviewHeight = EditorState
        ? std::clamp(EditorState->PreviewPaneHeight, FAnimationSequenceSequencerLayout::PreviewMinHeight, MaxPreviewHeight)
        : SplitLayoutHeight * 0.58f;
    const float SequencerHeight = std::max(
        FAnimationSequenceSequencerLayout::SequencerMinHeight,
        SplitLayoutHeight - PreviewHeight - FAnimationSequenceSequencerLayout::SectionSplitterHeight);

    if (EditorState)
    {
        EditorState->PreviewPaneHeight = PreviewHeight;
    }

    RenderPreviewPane(PreviewHeight, PreviewOverlayLines);

    if (EditorState)
    {
        RenderSequencerRegion(SequencerHeight, MaxPreviewHeight);
    }
}

//----------------------------------------------------------------------------
// Lower details/debug area
//----------------------------------------------------------------------------

void FAnimationSequenceEditorWidget::RenderSelectionDetailsPane(float Height)
{
    RenderNotifyLowerPanel(Height);
}

void FAnimationSequenceEditorWidget::RenderNotifyLowerPanel(float Height)
{
    ImGui::BeginChild("##AnimationSequenceSelectionDetails", ImVec2(0.0f, Height), true);
    RenderNotifyLowerPanelSplit();
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderNotifyLowerPanelSplit()
{
    if (ImGui::BeginTable("##AnimationSequenceSelectionDetailsTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Selection", ImGuiTableColumnFlags_WidthStretch, 0.55f);
        ImGui::TableSetupColumn("Activity", ImGuiTableColumnFlags_WidthStretch, 0.45f);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        if (ImGui::BeginChild("##AnimationSequenceSelectionColumn", ImVec2(0.0f, 0.0f), false))
        {
            RenderSelectedNotifyEditorPanel();
            RenderCurveInspectionPanel();
        }
        ImGui::EndChild();

        ImGui::TableSetColumnIndex(1);
        if (ImGui::BeginChild("##AnimationSequenceNotifyDebugColumn", ImVec2(0.0f, 0.0f), false))
        {
            RenderNotifyDebugPanel();
        }
        ImGui::EndChild();
        ImGui::EndTable();
    }
}

void FAnimationSequenceEditorWidget::RenderNotifyDetailsPanel()
{
    RenderSelectedNotifyEditorPanel();
}

void FAnimationSequenceEditorWidget::RenderSelectedNotifyEditorPanel()
{
    ImGui::TextDisabled("Selected Notify");
    ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing * 0.5f));

    if (!Document || !EditorState)
    {
        ImGui::TextDisabled("Notify editing is unavailable.");
        return;
    }

    const int32 DefaultTrackIndex = std::max(EditorState->SelectedNotifyTrackIndex, 0);
    const bool bHasSelection = Document->HasSelectedNotify();
    RenderSelectedNotifyToolbar(bHasSelection, DefaultTrackIndex);
    ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing * 0.5f));

    const FSelectedNotifyEditorContext Context = BuildSelectedNotifyEditorContext(Document);
    if (!Context.bValid)
    {
        NotifyDetailsBoundStableId.clear();
        NotifyNameEditBuffer.fill('\0');
        ResetNotifyPayloadFieldBuffers();
        NotifyPayloadEditBuffer.fill('\0');
        ImGui::TextDisabled("Select a notify marker to edit it.");
        return;
    }

    SyncNotifyDetailsBuffers(Context.NotifySnapshot);
    RenderSelectedNotifyBasicSection(Context.NotifySnapshot, Context.ValidationReport);
    RenderSelectedNotifyTimingSection(Context.NotifySnapshot, Context.ValidationReport);
    RenderSelectedNotifyPayloadSection(Context.NotifyClassName, Context.ValidationReport);
    RenderSelectedNotifyVisualSection(Context.NotifySnapshot);
    RenderSelectedNotifyInlineValidationSection(Context.ValidationReport);
    RenderSelectedNotifyMetaFooter(Context.NotifySnapshot);
}

void FAnimationSequenceEditorWidget::RenderSelectedNotifyToolbar(bool bHasSelection, int32 DefaultTrackIndex)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(NotifyPanelToolbarSpacing, NotifyPanelToolbarSpacing));

    if (ImGui::Button("Add Notify"))
    {
        Document->AddNotifyAtTime(DefaultTrackIndex, EditorState->CurrentTime);
    }

    ImGui::SameLine(0.0f, NotifyPanelToolbarSpacing);
    if (!bHasSelection) { ImGui::BeginDisabled(); }
    if (ImGui::Button("Duplicate")) { Document->DuplicateSelectedNotify(); }
    if (!bHasSelection) { ImGui::EndDisabled(); }

    ImGui::SameLine(0.0f, NotifyPanelToolbarSpacing);
    if (!bHasSelection) { ImGui::BeginDisabled(); }
    if (ImGui::Button("Copy")) { Document->CopySelectedNotify(); }
    if (!bHasSelection) { ImGui::EndDisabled(); }

    ImGui::SameLine(0.0f, NotifyPanelToolbarSpacing);
    if (!Document->CanPasteNotify()) { ImGui::BeginDisabled(); }
    if (ImGui::Button("Paste"))
    {
        Document->PasteNotifyToTrackAtTime(DefaultTrackIndex, EditorState->CurrentTime);
    }
    if (!Document->CanPasteNotify()) { ImGui::EndDisabled(); }

    ImGui::SameLine(0.0f, NotifyPanelToolbarSpacing);
    if (!bHasSelection) { ImGui::BeginDisabled(); }
    if (ImGui::Button("Delete Selected")) { Document->DeleteSelectedNotify(); }
    if (!bHasSelection) { ImGui::EndDisabled(); }

    ImGui::PopStyleVar();
}

void FAnimationSequenceEditorWidget::RenderSelectedNotifyBasicSection(
    const FAnimNotifyEvent& NotifySnapshot,
    const FAnimNotifyValidationReport& SelectedValidationReport)
{
    AnimationSequenceViewerUi::DrawSectionHeader(
        "Basic",
        NotifyPanelSectionSpacing * 0.35f,
        NotifyPanelSectionSpacing * 0.25f);

    SetNotifyPanelFieldWidth();
    if (ImGui::InputText("Name", NotifyNameEditBuffer.data(), NotifyNameEditBuffer.size()))
    {
        Document->SetSelectedNotifyName(FName(NotifyNameEditBuffer.data()));
    }
    RenderNotifyValidationIssues(SelectedValidationReport, EAnimNotifyValidationField::Name);

    const char* NotifyTypeLabels[] = { "Notify", "Notify State" };
    int32 NotifyTypeIndex = NotifySnapshot.EventType == EAnimNotifyEventType::NotifyState ? 1 : 0;
    SetNotifyPanelFieldWidth();
    if (ImGui::Combo("Type", &NotifyTypeIndex, NotifyTypeLabels, 2))
    {
        Document->SetSelectedNotifyType(
            NotifyTypeIndex == 1 ? EAnimNotifyEventType::NotifyState : EAnimNotifyEventType::Notify);
        NotifyDetailsBoundStableId.clear();
    }
    RenderNotifyValidationIssues(SelectedValidationReport, EAnimNotifyValidationField::Type);

    FAnimNotifyEvent CurrentNotifySnapshot;
    const bool bHasCurrentNotify = Document->TryGetSelectedNotifySnapshot(CurrentNotifySnapshot);
    const EAnimNotifyEventType CurrentNotifyType =
        bHasCurrentNotify ? CurrentNotifySnapshot.EventType : NotifySnapshot.EventType;
    const FString CurrentNotifyClassName =
        bHasCurrentNotify ? Document->GetSelectedNotifyResolvedClassName() : NotifySnapshot.GetResolvedNotifyClassName();
    const TArray<FString> NotifyClassOptions = CollectNotifyClassOptions(CurrentNotifyType);
    const bool bHasRegisteredNotifyClass = ContainsNotifyClassOption(NotifyClassOptions, CurrentNotifyClassName);
    const FString NotifyClassPreviewLabel =
        CurrentNotifyClassName.empty()
            ? FString("<None>")
            : (bHasRegisteredNotifyClass ? CurrentNotifyClassName : CurrentNotifyClassName + " (missing)");

    SetNotifyPanelFieldWidth();
    if (ImGui::BeginCombo("Notify Class", NotifyClassPreviewLabel.c_str()))
    {
        for (const FString& NotifyClassOption : NotifyClassOptions)
        {
            const bool bSelected = NotifyClassOption == CurrentNotifyClassName;
            if (ImGui::Selectable(NotifyClassOption.c_str(), bSelected))
            {
                Document->SetSelectedNotifyClassName(NotifyClassOption);
                NotifyDetailsBoundStableId.clear();
            }
            if (bSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (!bHasRegisteredNotifyClass && !CurrentNotifyClassName.empty())
    {
        ImGui::TextDisabled("Registered class not found: %s", CurrentNotifyClassName.c_str());
    }
    RenderNotifyValidationIssues(SelectedValidationReport, EAnimNotifyValidationField::NotifyClass);

    if (!NotifyClassOptions.empty())
    {
        ImGui::Indent(NotifyPanelSectionIndent);
        ImGui::TextDisabled("Test classes: %s / %s", "UAnimNotifyLog", "UAnimNotifyStateLog");
        ImGui::Unindent(NotifyPanelSectionIndent);
    }
}

void FAnimationSequenceEditorWidget::RenderSelectedNotifyTimingSection(
    const FAnimNotifyEvent& NotifySnapshot,
    const FAnimNotifyValidationReport& SelectedValidationReport)
{
    AnimationSequenceViewerUi::DrawSectionHeader(
        "Timing",
        NotifyPanelSectionSpacing * 0.35f,
        NotifyPanelSectionSpacing * 0.25f);

    ImGui::Indent(NotifyPanelSectionIndent);
    ImGui::LabelText("Track", "%d", EditorState->SelectedNotifyTrackIndex + 1);
    ImGui::Unindent(NotifyPanelSectionIndent);

    float NotifyTime = NotifySnapshot.Time;
    const float MaxTime = std::max(EditorState->SequenceLength, 0.0f);
    SetNotifyPanelFieldWidth();
    if (ImGui::DragFloat("Time", &NotifyTime, 0.001f, 0.0f, MaxTime, "%.3f s"))
    {
        Document->SetSelectedNotifyTime(NotifyTime, EditorState->bSnapToFrames);
    }

    float NotifyDuration = NotifySnapshot.Duration;
    SetNotifyPanelFieldWidth();
    if (ImGui::DragFloat("Duration", &NotifyDuration, 0.001f, 0.0f, MaxTime, "%.3f s"))
    {
        Document->SetSelectedNotifyDuration(NotifyDuration);
    }
    RenderNotifyValidationIssues(SelectedValidationReport, EAnimNotifyValidationField::Duration);
}

void FAnimationSequenceEditorWidget::RenderSelectedNotifyPayloadSection(
    const FString& CurrentNotifyClassName,
    const FAnimNotifyValidationReport& SelectedValidationReport)
{
    AnimationSequenceViewerUi::DrawSectionHeader(
        "Payload",
        NotifyPanelSectionSpacing * 0.35f,
        NotifyPanelSectionSpacing * 0.25f);

    if (HasAnimNotifyPayloadSchema(CurrentNotifyClassName))
    {
        RenderStructuredNotifyPayloadEditor(CurrentNotifyClassName, SelectedValidationReport);
    }
    else
    {
        RenderRawNotifyPayloadEditor(CurrentNotifyClassName, SelectedValidationReport, true);
    }
}

void FAnimationSequenceEditorWidget::RenderSelectedNotifyVisualSection(const FAnimNotifyEvent& NotifySnapshot)
{
    AnimationSequenceViewerUi::DrawSectionHeader(
        "Visual",
        NotifyPanelSectionSpacing * 0.35f,
        NotifyPanelSectionSpacing * 0.25f);

    float Color[4] =
    {
        NotifySnapshot.Color.R,
        NotifySnapshot.Color.G,
        NotifySnapshot.Color.B,
        NotifySnapshot.Color.A
    };
    SetNotifyPanelFieldWidth();
    if (ImGui::ColorEdit4("Color", Color))
    {
        Document->SetSelectedNotifyColor(FColor(
            std::clamp(Color[0], 0.0f, 1.0f),
            std::clamp(Color[1], 0.0f, 1.0f),
            std::clamp(Color[2], 0.0f, 1.0f),
            std::clamp(Color[3], 0.0f, 1.0f)));
    }
}

void FAnimationSequenceEditorWidget::RenderSelectedNotifyInlineValidationSection(
    const FAnimNotifyValidationReport& SelectedValidationReport) const
{
    AnimationSequenceViewerUi::DrawSectionHeader(
        "Validation",
        NotifyPanelSectionSpacing * 0.35f,
        NotifyPanelSectionSpacing * 0.25f);
    RenderNotifyValidationSummary(SelectedValidationReport);
    ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing * 0.25f));
    ImGui::Checkbox("Show Validation Details", &EditorState->bShowNotifyValidationDetails);
    ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing * 0.25f));
    RenderNotifyValidationIssues(SelectedValidationReport, EAnimNotifyValidationField::General);
}

void FAnimationSequenceEditorWidget::RenderSelectedNotifyMetaFooter(const FAnimNotifyEvent& NotifySnapshot) const
{
    ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing * 0.6f));
    ImGui::TextDisabled(
        "Track %d  StableId %s",
        EditorState->SelectedNotifyTrackIndex + 1,
        NotifySnapshot.StableId.ToString().c_str());
}

void FAnimationSequenceEditorWidget::RenderNotifyValidationSummary(const FAnimNotifyValidationReport& Report) const
{
    const ImVec4 SummaryColor = GetValidationSeverityColor(
        Report.ErrorCount > 0
            ? EAnimNotifyValidationSeverity::Error
            : (Report.WarningCount > 0
                ? EAnimNotifyValidationSeverity::Warning
                : EAnimNotifyValidationSeverity::Info));

    ImGui::TextColored(
        SummaryColor,
        "Validation: %s",
        FormatAnimNotifyValidationSummary(Report).c_str());
}

void FAnimationSequenceEditorWidget::RenderNotifyValidationIssues(
    const FAnimNotifyValidationReport& Report,
    EAnimNotifyValidationField Field) const
{
    if (!(EditorState && EditorState->bShowNotifyValidationDetails))
    {
        return;
    }

    for (const FAnimNotifyValidationIssue& Issue : Report.Issues)
    {
        if (Issue.Field != Field)
        {
            continue;
        }

        const ImVec4 SeverityColor = GetValidationSeverityColor(Issue.Severity);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(
            SeverityColor,
            "%s: %s",
            GetValidationSeverityLabel(Issue.Severity),
            Issue.Message.c_str());
        if (!Issue.Hint.empty())
        {
            ImGui::TextColored(
                ImVec4(SeverityColor.x, SeverityColor.y, SeverityColor.z, 0.82f),
                "Hint: %s",
                Issue.Hint.c_str());
        }
        ImGui::PopTextWrapPos();
    }
}

void FAnimationSequenceEditorWidget::RenderStructuredNotifyPayloadEditor(
    const FString& NotifyClassName,
    const FAnimNotifyValidationReport& Report)
{
    if (!Document)
    {
        return;
    }

    const FAnimNotifyPayloadSchema* Schema = FindAnimNotifyPayloadSchema(NotifyClassName);
    if (!Schema)
    {
        RenderRawNotifyPayloadEditor(NotifyClassName, Report, true);
        return;
    }

    const FAnimNotifyEvent* CurrentNotify = Document->GetSelectedNotify();
    const FAnimNotifyPayloadParser Payload = Document->GetSelectedNotifyPayloadParser();

    auto RefreshPayloadBuffers = [this]()
    {
        if (const FAnimNotifyEvent* UpdatedNotify = Document ? Document->GetSelectedNotify() : nullptr)
        {
            CopyStringToBuffer(UpdatedNotify->Payload, NotifyPayloadEditBuffer);
            const FAnimNotifyPayloadParser UpdatedPayload(UpdatedNotify->Payload);
            RefreshNotifyPayloadFieldBuffers(UpdatedPayload);
        }
    };

    ImGui::TextDisabled("%s Payload", Schema->SectionLabel.c_str());
    for (const FAnimNotifyPayloadFieldDefinition& Field : Schema->Fields)
    {
        auto ApplyTextFieldValue = [&](const FString& Value)
        {
            return Field.Type == EAnimNotifyPayloadFieldType::Name
                ? (Value.empty()
                    ? Document->ClearSelectedNotifyPayloadValue(Field.Key)
                    : Document->SetSelectedNotifyPayloadNameValue(Field.Key, FName(Value)))
                : (Value.empty()
                    ? Document->ClearSelectedNotifyPayloadValue(Field.Key)
                    : Document->SetSelectedNotifyPayloadStringValue(Field.Key, Value));
        };

        switch (Field.Type)
        {
        case EAnimNotifyPayloadFieldType::String:
        case EAnimNotifyPayloadFieldType::Name:
        {
            std::array<char, 256>* TargetBuffer = GetNotifyTextFieldBuffer(Field.SemanticId);
            if (!TargetBuffer)
            {
                continue;
            }

            TArray<FString> PickerOptions;
            if (PreviewController)
            {
                if (Field.EditorHint == EAnimNotifyPayloadFieldEditorHint::SocketPicker)
                {
                    PickerOptions = PreviewController->GetPreviewSocketNames();
                }
                else if (Field.EditorHint == EAnimNotifyPayloadFieldEditorHint::ComponentPicker)
                {
                    PickerOptions = PreviewController->GetPreviewPrimitiveComponentNames();
                }
            }

            if (!PickerOptions.empty())
            {
                const FString CurrentValue = TargetBuffer->data();
                const char* PreviewValue = CurrentValue.empty() ? "(None)" : CurrentValue.c_str();
                SetNotifyPanelFieldWidth();
                if (ImGui::BeginCombo(Field.Label.c_str(), PreviewValue))
                {
                    const bool bIsNoneSelected = CurrentValue.empty();
                    if (ImGui::Selectable("(None)", bIsNoneSelected))
                    {
                        if (ApplyTextFieldValue(FString()))
                        {
                            RefreshPayloadBuffers();
                        }
                    }

                    for (const FString& Option : PickerOptions)
                    {
                        const bool bIsSelected = CurrentValue == Option;
                        if (ImGui::Selectable(Option.c_str(), bIsSelected))
                        {
                            if (ApplyTextFieldValue(Option))
                            {
                                RefreshPayloadBuffers();
                            }
                        }

                        if (bIsSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                    ImGui::EndCombo();
                }

                ImGui::TextDisabled("Pick from the active preview context. Use Advanced Raw Payload for a manual override.");
            }
            else
            {
                SetNotifyPanelFieldWidth();
                if (ImGui::InputText(Field.Label.c_str(), TargetBuffer->data(), TargetBuffer->size()))
                {
                    const FString Value = TargetBuffer->data();
                    if (ApplyTextFieldValue(Value))
                    {
                        RefreshPayloadBuffers();
                    }
                }

                if (const char* UnavailableMessage = GetPickerUnavailableMessage(Field.EditorHint))
                {
                    ImGui::TextDisabled("%s", UnavailableMessage);
                }
            }
            break;
        }

        case EAnimNotifyPayloadFieldType::Float:
        {
            float FieldValue = GetSchemaFloatValue(Payload, Field);
            SetNotifyPanelFieldWidth();
            if (ImGui::DragFloat(Field.Label.c_str(), &FieldValue, 0.01f, 0.0f, 100.0f, "%.3f"))
            {
                if (Document->SetSelectedNotifyPayloadFloatValue(Field.Key, FieldValue))
                {
                    RefreshPayloadBuffers();
                }
            }
            break;
        }

        case EAnimNotifyPayloadFieldType::Int:
        {
            int32 FieldValue = GetSchemaIntValue(Payload, Field);
            SetNotifyPanelFieldWidth();
            if (ImGui::InputInt(Field.Label.c_str(), &FieldValue))
            {
                FAnimNotifyPayloadParser UpdatedPayload = Document->GetSelectedNotifyPayloadParser();
                UpdatedPayload.RemoveAny(Field.LegacyKeys);
                UpdatedPayload.SetInt(Field.Key, FieldValue);
                if (Document->SetSelectedNotifyPayload(UpdatedPayload.SerializeCanonical()))
                {
                    RefreshPayloadBuffers();
                }
            }
            break;
        }

        case EAnimNotifyPayloadFieldType::Bool:
        {
            bool FieldValue = GetSchemaBoolValue(Payload, Field);
            if (ImGui::Checkbox(Field.Label.c_str(), &FieldValue))
            {
                if (Document->SetSelectedNotifyPayloadBoolValue(Field.Key, FieldValue))
                {
                    RefreshPayloadBuffers();
                }
            }
            break;
        }

        default:
            break;
        }

        if (!Field.HelpText.empty())
        {
            ImGui::Indent(NotifyPanelSectionIndent);
            ImGui::TextDisabled("%s", Field.HelpText.c_str());
            ImGui::Unindent(NotifyPanelSectionIndent);
        }
    }

    const FString PayloadPreview = CurrentNotify
        ? FAnimNotifyPayloadParser(CurrentNotify->Payload).SerializeCanonical()
        : Payload.SerializeCanonical();
    ImGui::TextWrapped(
        "Payload Preview: %s",
        PayloadPreview.empty() ? "(Empty)" : PayloadPreview.c_str());

    RenderNotifyValidationIssues(Report, EAnimNotifyValidationField::Payload);

    if (ImGui::CollapsingHeader("Advanced Raw Payload"))
    {
        RenderRawNotifyPayloadEditor(NotifyClassName, Report, false);
    }
}

void FAnimationSequenceEditorWidget::RenderRawNotifyPayloadEditor(
    const FString& NotifyClassName,
    const FAnimNotifyValidationReport& Report,
    bool bRenderValidation)
{
    if (!Document)
    {
        return;
    }

    SetNotifyPanelFieldWidth();
    if (ImGui::InputText("Payload", NotifyPayloadEditBuffer.data(), NotifyPayloadEditBuffer.size()))
    {
        Document->SetSelectedNotifyPayload(NotifyPayloadEditBuffer.data());
        const FAnimNotifyPayloadParser Payload(NotifyPayloadEditBuffer.data());
        RefreshNotifyPayloadFieldBuffers(Payload);
    }

    if (const char* PayloadExample = GetAnimNotifyPayloadExample(NotifyClassName))
    {
        ImGui::TextWrapped("Payload Example: %s", PayloadExample);
    }

    if (bRenderValidation)
    {
        RenderNotifyValidationIssues(Report, EAnimNotifyValidationField::Payload);
    }
}

void FAnimationSequenceEditorWidget::RenderNotifyValidationBrowser()
{
    RenderNotifyValidationTab();
}

void FAnimationSequenceEditorWidget::RenderNotifyDebugPanel()
{
    ImGui::TextDisabled("Notify Debug");
    ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing * 0.5f));
    RenderNotifyDebugTabs();
}

void FAnimationSequenceEditorWidget::RenderNotifyDebugTabs()
{
    if (!ImGui::BeginTabBar("##NotifyDebugTabs"))
    {
        return;
    }

    if (ImGui::BeginTabItem("Validation"))
    {
        RenderNotifyValidationBrowser();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Recent Fired"))
    {
        RenderRecentNotifySummary();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Event Log"))
    {
        RenderNotifyEventLogTab();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}

void FAnimationSequenceEditorWidget::RenderNotifyValidationTab()
{
    if (!Document)
    {
        ImGui::TextDisabled("Validation browser is unavailable.");
        return;
    }

    const FAnimNotifyValidationReport Report = Document->BuildDocumentNotifyValidationReport();
    RenderNotifyValidationSummaryBlock(Report);
    ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing * 0.4f));
    RenderNotifyValidationFilters();
    ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing * 0.5f));
    RenderNotifyValidationMessageList(Report);
}

void FAnimationSequenceEditorWidget::RenderNotifyValidationSummaryBlock(const FAnimNotifyValidationReport& Report) const
{
    AnimationSequenceViewerUi::DrawValidationSeverityCounts(
        Report.ErrorCount,
        Report.WarningCount,
        Report.InfoCount);
}

void FAnimationSequenceEditorWidget::RenderNotifyValidationFilters()
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(NotifyPanelToolbarSpacing, NotifyPanelToolbarSpacing));

    auto DrawSeverityFilterButton = [this](const char* Label, ENotifyValidationBrowserSeverityFilter Filter)
    {
        const bool bSelected = ActiveNotifyValidationSeverityFilter == Filter;
        if (bSelected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.32f, 0.44f, 0.85f));
        }

        const bool bClicked = ImGui::Button(Label);
        if (bSelected)
        {
            ImGui::PopStyleColor();
        }

        if (bClicked)
        {
            ActiveNotifyValidationSeverityFilter = Filter;
        }
    };

    DrawSeverityFilterButton("All", ENotifyValidationBrowserSeverityFilter::All);
    ImGui::SameLine();
    DrawSeverityFilterButton("Errors", ENotifyValidationBrowserSeverityFilter::Errors);
    ImGui::SameLine();
    DrawSeverityFilterButton("Warnings", ENotifyValidationBrowserSeverityFilter::Warnings);
    ImGui::SameLine();
    DrawSeverityFilterButton("Info", ENotifyValidationBrowserSeverityFilter::Info);

    ImGui::PopStyleVar();
}

void FAnimationSequenceEditorWidget::RenderNotifyValidationMessageList(const FAnimNotifyValidationReport& Report)
{
    auto MatchesSeverityFilter = [this](EAnimNotifyValidationSeverity Severity)
    {
        switch (ActiveNotifyValidationSeverityFilter)
        {
        case ENotifyValidationBrowserSeverityFilter::Errors:
            return Severity == EAnimNotifyValidationSeverity::Error;
        case ENotifyValidationBrowserSeverityFilter::Warnings:
            return Severity == EAnimNotifyValidationSeverity::Warning;
        case ENotifyValidationBrowserSeverityFilter::Info:
            return Severity == EAnimNotifyValidationSeverity::Info;
        case ENotifyValidationBrowserSeverityFilter::All:
        default:
            return true;
        }
    };

    TArray<const FAnimNotifyValidationIssue*> FilteredIssues;
    FilteredIssues.reserve(Report.Issues.size());
    for (const FAnimNotifyValidationIssue& Issue : Report.Issues)
    {
        if (MatchesSeverityFilter(Issue.Severity))
        {
            FilteredIssues.push_back(&Issue);
        }
    }

    std::stable_sort(
        FilteredIssues.begin(),
        FilteredIssues.end(),
        [](const FAnimNotifyValidationIssue* Left, const FAnimNotifyValidationIssue* Right)
        {
            if (Left->Severity != Right->Severity)
            {
                return static_cast<int32>(Left->Severity) > static_cast<int32>(Right->Severity);
            }
            if (Left->TrackIndex != Right->TrackIndex)
            {
                return Left->TrackIndex < Right->TrackIndex;
            }
            return Left->EventIndex < Right->EventIndex;
        });

    if (FilteredIssues.empty())
    {
        AnimationSequenceViewerUi::DrawCompactEmptyState(
            Report.Issues.empty()
                ? "No notify validation issues."
                : "No issues match the current severity filter.");
        return;
    }

    if (ImGui::BeginChild("##NotifyValidationBrowser", ImVec2(0.0f, 0.0f), true))
    {
        for (int32 IssueIndex = 0; IssueIndex < static_cast<int32>(FilteredIssues.size()); ++IssueIndex)
        {
            if (IssueIndex > 0)
            {
                ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing * 0.4f));
            }

            const FAnimNotifyValidationIssue& Issue = *FilteredIssues[IssueIndex];
            FString TrackLabel = Issue.TrackIndex >= 0
                ? ("Track " + std::to_string(Issue.TrackIndex + 1))
                : FString("Document");
            if (Issue.TrackIndex >= 0)
            {
                if (const FAnimNotifyTrack* Track = Document->GetNotifyTrack(Issue.TrackIndex))
                {
                    const FString TrackName = Track->TrackName.IsValid() ? Track->TrackName.ToString() : FString();
                    if (!TrackName.empty())
                    {
                        TrackLabel += " (" + TrackName + ")";
                    }
                }
            }

            const FString RowLabel =
                "[" + FString(GetValidationSeverityLabel(Issue.Severity)) + "] " +
                TrackLabel + " - " + Issue.Message +
                "##NotifyValidationIssue" + std::to_string(IssueIndex);

            ImGui::PushStyleColor(ImGuiCol_Text, GetValidationSeverityColor(Issue.Severity));
            const bool bRowClicked = ImGui::Selectable(RowLabel.c_str(), false);
            ImGui::PopStyleColor();

            if (bRowClicked && Issue.TrackIndex >= 0 && Issue.StableId.IsValid())
            {
                Document->SelectNotifyByStableId(Issue.TrackIndex, Issue.StableId);
            }

            if (Issue.EventIndex >= 0)
            {
                ImGui::Indent();
                ImGui::TextDisabled("Event %d", Issue.EventIndex + 1);
                ImGui::Unindent();
            }

            if (!Issue.Hint.empty())
            {
                ImGui::Indent();
                ImGui::PushTextWrapPos(0.0f);
                ImGui::TextDisabled("%s", Issue.Hint.c_str());
                ImGui::PopTextWrapPos();
                ImGui::Unindent();
            }
        }
    }
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderRecentNotifySummary() const
{
    RenderRecentFiredNotifyTab();
}

void FAnimationSequenceEditorWidget::RenderRecentFiredNotifyTab() const
{
    if (!CanInspectRecentFiredNotifies(PreviewController))
    {
        ImGui::TextDisabled("Preview controller is unavailable.");
        return;
    }

    const TArray<FAnimNotifyEvent>& RecentNotifies = PreviewController->GetRecentFiredNotifyEvents();
    if (RecentNotifies.empty())
    {
        if (ImGui::BeginChild("##RecentFiredNotifyTabEmpty", ImVec2(0.0f, 0.0f), true))
        {
            AnimationSequenceViewerUi::DrawCompactEmptyState(
                "No notify fired during the latest preview update.",
                "Preview playback activity will appear here.",
                NotifyPanelSectionSpacing);
        }
        ImGui::EndChild();
        return;
    }

    if (ImGui::BeginChild("##RecentFiredNotifyTab", ImVec2(0.0f, 0.0f), true))
    {
        constexpr int32 MaxVisibleEntries = 6;
        const int32 VisibleCount = std::min(static_cast<int32>(RecentNotifies.size()), MaxVisibleEntries);
        for (int32 Offset = 0; Offset < VisibleCount; ++Offset)
        {
            if (Offset > 0)
            {
                ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing * 0.5f));
            }

            const int32 RecentIndex = static_cast<int32>(RecentNotifies.size()) - 1 - Offset;
            const FAnimNotifyEvent& NotifyEvent = RecentNotifies[RecentIndex];
            const FString NotifyName = NotifyEvent.Name.IsValid() ? NotifyEvent.Name.ToString() : FString("(Unnamed)");
            const FString PhaseText = NotifyEvent.TriggerPhase == EAnimNotifyTriggerPhase::None
                ? AnimNotifyEventTypeToString(NotifyEvent.EventType)
                : AnimNotifyTriggerPhaseToString(NotifyEvent.TriggerPhase);
            const FString NotifyClassName = NotifyEvent.GetResolvedNotifyClassName();
            const FString TrackLabel = MakeRecentNotifyTrackLabel(NotifyEvent);
            const FString SourceLabel = MakeRecentNotifySourceLabel(NotifyEvent);

            ImGui::BulletText("%s [%s] @ %.3fs", NotifyName.c_str(), PhaseText.c_str(), NotifyEvent.Time);
            ImGui::Indent();
            ImGui::TextDisabled(
                "Class: %s | Track: %s | Source: %s",
                NotifyClassName.c_str(),
                TrackLabel.c_str(),
                SourceLabel.c_str());
            if (!NotifyEvent.Payload.empty())
            {
                ImGui::PushTextWrapPos(0.0f);
                ImGui::TextDisabled("Payload: %s", NotifyEvent.Payload.c_str());
                ImGui::PopTextWrapPos();
            }
            ImGui::Unindent();
        }
    }
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderNotifyEventLogTab() const
{
    if (ImGui::BeginChild("##NotifyEventLogPlaceholder", ImVec2(0.0f, 0.0f), true))
    {
        AnimationSequenceViewerUi::DrawCompactEmptyState(
            "Notify event log is reserved for future debug output.",
            "Use Validation and Recent Fired for the current notify debug flow.",
            NotifyPanelSectionSpacing);
    }
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderCurveInspectionPanel()
{
    ImGui::Spacing();
    ImGui::TextDisabled("Selected Curve");

    const FCurveInspectionContext Context = BuildCurveInspectionContext(Sequence, EditorState);
    if (!Context.DataModel || Context.DataModel->CurveData.FloatCurves.empty())
    {
        ImGui::TextDisabled("No float curves in this sequence.");
        return;
    }

    if (!Context.SelectedCurve)
    {
        ImGui::TextDisabled(
            "%d visible curves / %d total. Select a curve from the outliner or graph row.",
            Context.VisibleCurveCount,
            static_cast<int32>(Context.DataModel->CurveData.FloatCurves.size()));
        for (const FAnimationSequenceCurveViewGroup& Group : Context.CurveGroups)
        {
            ImGui::BulletText(
                "%s: %d %s",
                Group.Label.c_str(),
                Group.TotalCount,
                Group.bVisible ? "shown" : "hidden");
        }
        return;
    }

    if (!EditorState)
    {
        ImGui::TextDisabled("Curve inspection state is unavailable.");
        return;
    }

    const FFloatCurve& SelectedCurve = *Context.SelectedCurve;
    const FCurveInspectionSnapshot& Snapshot = Context.Snapshot;

    const bool bCanFrameSelection = Snapshot.FirstKey.bValid && Snapshot.LastKey.bValid;
    if (!bCanFrameSelection) { ImGui::BeginDisabled(); }
    if (ImGui::Button("Frame Selection") && bCanFrameSelection)
    {
        float FrameStart = Snapshot.FirstKey.Time;
        float FrameEnd = Snapshot.LastKey.Time;
        const float MinimumVisibleRange = EditorState->GetMinimumVisibleRange();
        if (FrameEnd - FrameStart < MinimumVisibleRange)
        {
            const float CenterTime = (FrameStart + FrameEnd) * 0.5f;
            FrameStart = CenterTime - MinimumVisibleRange * 0.5f;
            FrameEnd = CenterTime + MinimumVisibleRange * 0.5f;
        }

        EditorState->SetVisibleRange(FrameStart, FrameEnd);
    }
    if (!bCanFrameSelection) { ImGui::EndDisabled(); }

    ImGui::SameLine();
    if (ImGui::Button("Solo Type"))
    {
        AnimationSequenceCurveFilter::SetExclusiveCurveTypeVisible(*EditorState, SelectedCurve.CurveType);
    }

    ImGui::SameLine();
    if (ImGui::Button("Show All"))
    {
        AnimationSequenceCurveFilter::SetAllCurveTypesEnabled(*EditorState, true);
    }

    ImGui::BulletText("%s", SelectedCurve.CurveName.ToString().c_str());
    ImGui::BulletText("Type %s", AnimationCurveTypeToString(SelectedCurve.CurveType).c_str());
    ImGui::BulletText("Source %s", AnimationCurveSourceKindToString(SelectedCurve.SourceKind).c_str());
    ImGui::BulletText("%d keys", static_cast<int32>(SelectedCurve.Keys.size()));
    ImGui::BulletText("Current Value %.3f @ %.3fs", Snapshot.CurrentValue, EditorState->CurrentTime);

    if (Snapshot.FirstKey.bValid && Snapshot.LastKey.bValid)
    {
        ImGui::BulletText("Time Range %.3fs - %.3fs", Snapshot.FirstKey.Time, Snapshot.LastKey.Time);
        ImGui::BulletText("First Key #%d  %.3fs  %.3f", Snapshot.FirstKey.KeyIndex, Snapshot.FirstKey.Time, Snapshot.FirstKey.Value);
        ImGui::BulletText("Last Key #%d  %.3fs  %.3f", Snapshot.LastKey.KeyIndex, Snapshot.LastKey.Time, Snapshot.LastKey.Value);
    }
    else
    {
        ImGui::TextDisabled("No curve keys.");
    }

    if (Snapshot.FullValueRange.bValid)
    {
        ImGui::BulletText("Value Range %.3f - %.3f", Snapshot.FullValueRange.MinValue, Snapshot.FullValueRange.MaxValue);
    }

    if (Snapshot.VisibleValueRange.bValid)
    {
        ImGui::BulletText(
            "Visible Value Range %.3f - %.3f",
            Snapshot.VisibleValueRange.MinValue,
            Snapshot.VisibleValueRange.MaxValue);
    }

    if (Snapshot.NearestKey.bValid)
    {
        ImGui::BulletText(
            "Nearest Key #%d  %.3fs  %.3f  (delta %.3fs)",
            Snapshot.NearestKey.KeyIndex,
            Snapshot.NearestKey.Time,
            Snapshot.NearestKey.Value,
            std::fabs(Snapshot.NearestKey.Time - EditorState->CurrentTime));
    }

    if (EditorState->bHasHoveredCurveSample && EditorState->HoveredCurveIndex == EditorState->SelectedCurveIndex)
    {
        ImGui::BulletText(
            "Hovered Sample %.3f @ %.3fs",
            EditorState->HoveredCurveSampleValue,
            EditorState->HoveredCurveSampleTime);
        if (EditorState->HoveredCurveNearestKeyIndex >= 0 &&
            EditorState->HoveredCurveNearestKeyIndex < static_cast<int32>(SelectedCurve.Keys.size()))
        {
            const FCurveKey& HoveredKey = SelectedCurve.Keys[EditorState->HoveredCurveNearestKeyIndex];
            ImGui::BulletText(
                "Hovered Nearest Key #%d  %.3fs  %.3f",
                EditorState->HoveredCurveNearestKeyIndex,
                HoveredKey.Time,
                HoveredKey.Value);
        }
    }
}

//----------------------------------------------------------------------------
// Shared viewer helpers and notify editor buffer sync
//----------------------------------------------------------------------------

void FAnimationSequenceEditorWidget::ResetNotifyPayloadFieldBuffers()
{
    NotifySoundCueEditBuffer.fill('\0');
    NotifySocketNameEditBuffer.fill('\0');
    NotifyComponentNameEditBuffer.fill('\0');
    NotifyAttackIdEditBuffer.fill('\0');
    NotifyGameplayEventNameEditBuffer.fill('\0');
    NotifyGameplayEndEventNameEditBuffer.fill('\0');
    NotifyGameplayPayloadValueEditBuffer.fill('\0');
    NotifyVFXEffectEditBuffer.fill('\0');
    NotifyDecalEditBuffer.fill('\0');
    NotifyCameraShakeEditBuffer.fill('\0');
}

void FAnimationSequenceEditorWidget::RefreshNotifyPayloadFieldBuffers(const FAnimNotifyPayloadParser& Payload)
{
    const FAnimNotifyPayloadFieldDefinition SoundCueField =
    {
        AnimNotifySemanticFieldNames::SoundCueKey(),
        EAnimNotifySemanticFieldId::SoundCue,
        AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::SoundCueKey())
    };
    const FAnimNotifyPayloadFieldDefinition SocketNameField =
    {
        AnimNotifySemanticFieldNames::SocketNameKey(),
        EAnimNotifySemanticFieldId::SocketName,
        AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::SocketNameKey())
    };
    const FAnimNotifyPayloadFieldDefinition ComponentNameField =
    {
        AnimNotifySemanticFieldNames::ComponentNameKey(),
        EAnimNotifySemanticFieldId::ComponentName,
        AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::ComponentNameKey())
    };
    const FAnimNotifyPayloadFieldDefinition AttackIdField =
    {
        AnimNotifySemanticFieldNames::AttackIdKey(),
        EAnimNotifySemanticFieldId::AttackId,
        AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::AttackIdKey())
    };
    const FAnimNotifyPayloadFieldDefinition EventNameField =
    {
        AnimNotifySemanticFieldNames::EventNameKey(),
        EAnimNotifySemanticFieldId::EventName,
        AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::EventNameKey())
    };
    const FAnimNotifyPayloadFieldDefinition PayloadField =
    {
        AnimNotifySemanticFieldNames::PayloadKey(),
        EAnimNotifySemanticFieldId::PayloadValue,
        AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::PayloadKey())
    };
    const FAnimNotifyPayloadFieldDefinition EndEventNameField =
    {
        AnimNotifySemanticFieldNames::EndEventNameKey(),
        EAnimNotifySemanticFieldId::EndEventName,
        AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::EndEventNameKey())
    };
    const FAnimNotifyPayloadFieldDefinition ShakeField =
    {
        AnimNotifySemanticFieldNames::ShakeKey(),
        EAnimNotifySemanticFieldId::Shake,
        AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::ShakeKey())
    };
    const FAnimNotifyPayloadFieldDefinition EffectField =
    {
        AnimNotifySemanticFieldNames::EffectKey(),
        EAnimNotifySemanticFieldId::Effect,
        AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::EffectKey())
    };
    const FAnimNotifyPayloadFieldDefinition DecalField =
    {
        AnimNotifySemanticFieldNames::DecalKey(),
        EAnimNotifySemanticFieldId::Decal,
        AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::DecalKey())
    };

    CopyPayloadFieldToBuffer(Payload, SoundCueField, NotifySoundCueEditBuffer);
    CopyPayloadFieldToBuffer(Payload, SocketNameField, NotifySocketNameEditBuffer);
    CopyPayloadFieldToBuffer(Payload, ComponentNameField, NotifyComponentNameEditBuffer);
    CopyPayloadFieldToBuffer(Payload, AttackIdField, NotifyAttackIdEditBuffer);
    CopyPayloadFieldToBuffer(Payload, EventNameField, NotifyGameplayEventNameEditBuffer);
    CopyPayloadFieldToBuffer(Payload, EndEventNameField, NotifyGameplayEndEventNameEditBuffer);
    CopyPayloadFieldToBuffer(Payload, PayloadField, NotifyGameplayPayloadValueEditBuffer);
    CopyPayloadFieldToBuffer(Payload, EffectField, NotifyVFXEffectEditBuffer);
    CopyPayloadFieldToBuffer(Payload, DecalField, NotifyDecalEditBuffer);
    CopyPayloadFieldToBuffer(Payload, ShakeField, NotifyCameraShakeEditBuffer);
}

std::array<char, 256>* FAnimationSequenceEditorWidget::GetNotifyTextFieldBuffer(EAnimNotifySemanticFieldId SemanticId)
{
    switch (SemanticId)
    {
    case EAnimNotifySemanticFieldId::SoundCue:
        return &NotifySoundCueEditBuffer;
    case EAnimNotifySemanticFieldId::SocketName:
        return &NotifySocketNameEditBuffer;
    case EAnimNotifySemanticFieldId::ComponentName:
        return &NotifyComponentNameEditBuffer;
    case EAnimNotifySemanticFieldId::AttackId:
        return &NotifyAttackIdEditBuffer;
    case EAnimNotifySemanticFieldId::EventName:
        return &NotifyGameplayEventNameEditBuffer;
    case EAnimNotifySemanticFieldId::EndEventName:
        return &NotifyGameplayEndEventNameEditBuffer;
    case EAnimNotifySemanticFieldId::PayloadValue:
        return &NotifyGameplayPayloadValueEditBuffer;
    case EAnimNotifySemanticFieldId::Effect:
        return &NotifyVFXEffectEditBuffer;
    case EAnimNotifySemanticFieldId::Decal:
        return &NotifyDecalEditBuffer;
    case EAnimNotifySemanticFieldId::Shake:
        return &NotifyCameraShakeEditBuffer;
    default:
        return nullptr;
    }
}

void FAnimationSequenceEditorWidget::SyncNotifyDetailsBuffers(const FAnimNotifyEvent& NotifyEvent)
{
    const FString StableId = NotifyEvent.StableId.ToString();
    if (NotifyDetailsBoundStableId == StableId)
    {
        return;
    }

    NotifyDetailsBoundStableId = StableId;
    const FString NotifyName = NotifyEvent.Name.IsValid() ? NotifyEvent.Name.ToString() : FString();
    CopyStringToBuffer(NotifyName, NotifyNameEditBuffer);
    CopyStringToBuffer(NotifyEvent.Payload, NotifyPayloadEditBuffer);

    const FAnimNotifyPayloadParser Payload(NotifyEvent.Payload);
    RefreshNotifyPayloadFieldBuffers(Payload);
}
