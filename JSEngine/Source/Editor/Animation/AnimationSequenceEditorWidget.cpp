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
#include "Editor/Animation/AnimationSequenceTimelineGeometry.h"
#include "Editor/Animation/AnimationSequenceViewerUtils.h"
#include "Editor/EditorEngine.h"
#include "Editor/Viewport/EditorViewportClient.h"
#include "Editor/Viewport/FSceneViewport.h"
#include "Core/ResourceManager.h"
#include "Core/Paths.h"
#include "Engine/Asset/CurveFloatAsset.h"
#include "Render/Resource/Texture.h"
#include "Object/ObjectFactory.h"
#include "Engine/Runtime/WindowsWindow.h"
#include "Render/Common/RenderTypes.h"

#include "ImGui/imgui.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

namespace
{
    constexpr float PreviewOverlayMargin = 10.0f;
    constexpr float PreviewOverlayPadding = 8.0f;
    constexpr float PreviewOverlayLineSpacing = 2.0f;

    constexpr float SequencerHeaderPadding = 8.0f;
    constexpr float PlaybackIconButtonSize = 14.0f;
    constexpr float PlaybackControlSpacing = 2.0f;
    constexpr float NotifyPanelSectionSpacing = 10.0f;
    constexpr float NotifyPanelSectionIndent = 6.0f;
    constexpr float NotifyPanelToolbarSpacing = 6.0f;
    constexpr float NotifyPanelFieldMinWidth = 220.0f;
    constexpr float NotifyPanelFieldMaxWidth = 440.0f;

    float GetNotifyPanelFieldWidth()
    {
        return std::clamp(
            ImGui::GetContentRegionAvail().x * 0.72f,
            NotifyPanelFieldMinWidth,
            NotifyPanelFieldMaxWidth);
    }

    void SetNotifyPanelFieldWidth()
    {
        ImGui::SetNextItemWidth(GetNotifyPanelFieldWidth());
    }

    void DrawNotifyPanelSectionHeader(const char* Label)
    {
        ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing * 0.35f));
        ImGui::TextDisabled("%s", Label);
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing * 0.25f));
    }

    struct FTransportStripVerticalCenter
    {
        float ContentTopY = 0.0f;
        float ContentHeight = 0.0f;

        float ComputeCursorY(float ItemHeight) const
        {
            return ContentTopY + std::max(0.0f, (ContentHeight - ItemHeight) * 0.5f);
        }

        void AlignCursorY(float ItemHeight) const
        {
            ImGui::SetCursorPosY(ComputeCursorY(ItemHeight));
        }

        ImVec2 ComputeCenteredTextPos(const ImVec2& Min, const ImVec2& Max, const ImVec2& TextSize) const
        {
            return ImVec2(
                std::floor(Min.x + ((Max.x - Min.x) - TextSize.x) * 0.5f),
                std::floor(Min.y + ((Max.y - Min.y) - TextSize.y) * 0.5f));
        }
    };

    float GetTrackOutlinerFilterHeight()
    {
        return
            FAnimationSequenceTimelineGeometry::VerticalPadding +
            FAnimationSequenceSequencerLayout::RulerHeight +
            FAnimationSequenceSequencerLayout::TrackAreaPadding;
    }

    const FString& GetPlaybackIconPathJumpToStart()
    {
        static const FString Path = "Asset/UI/PlaybackControls/PlayControlsToFront.png";
        return Path;
    }

    const FString& GetPlaybackIconPathStepPrevious()
    {
        static const FString Path = "Asset/UI/PlaybackControls/PlayControlsToPrevious.png";
        return Path;
    }

    const FString& GetPlaybackIconPathPlayReverse()
    {
        static const FString Path = "Asset/UI/PlaybackControls/PlayControlsPlayReverse.png";
        return Path;
    }

    const FString& GetPlaybackIconPathRecord()
    {
        static const FString Path = "Asset/UI/PlaybackControls/PlayControlsRecord.png";
        return Path;
    }

    const FString& GetPlaybackIconPathPlayForward()
    {
        static const FString Path = "Asset/UI/PlaybackControls/PlayControlsPlayForward.png";
        return Path;
    }

    const FString& GetPlaybackIconPathPause()
    {
        static const FString Path = "Asset/UI/PlaybackControls/PlayControlsPause.png";
        return Path;
    }

    const FString& GetPlaybackIconPathStop()
    {
        static const FString Path = "Asset/UI/PlaybackControls/PlayControlsStop.png";
        return Path;
    }

    const FString& GetPlaybackIconPathStepNext()
    {
        static const FString Path = "Asset/UI/PlaybackControls/PlayControlsToNext.png";
        return Path;
    }

    const FString& GetPlaybackIconPathJumpToEnd()
    {
        static const FString Path = "Asset/UI/PlaybackControls/PlayControlsToEnd.png";
        return Path;
    }

    const FString& GetPlaybackIconPathLoopEnabled()
    {
        static const FString Path = "Asset/UI/PlaybackControls/PlayControlsLooping.png";
        return Path;
    }

    const FString& GetPlaybackIconPathLoopDisabled()
    {
        static const FString Path = "Asset/UI/PlaybackControls/PlayControlsNoLooping.png";
        return Path;
    }

    FString FormatPlaybackRateLabel(float PlaybackRate)
    {
        char Buffer[32];
        snprintf(Buffer, sizeof(Buffer), "x%.2f", std::fabs(PlaybackRate));

        FString Label = Buffer;
        while (Label.size() > 3 && !Label.empty() && Label.back() == '0')
        {
            Label.pop_back();
        }

        if (!Label.empty() && Label.back() == '.')
        {
            Label.push_back('0');
        }

        return Label;
    }

    FString FormatCompactTimelineRangeLabel(const FAnimationSequenceEditorState& State, float Time)
    {
        if (State.TotalFrames > 0)
        {
            return std::to_string(State.TimeToFrameIndex(Time));
        }

        char Buffer[32];
        snprintf(Buffer, sizeof(Buffer), "%.3f", Time);
        FString Label = Buffer;
        while (Label.size() > 2 && !Label.empty() && Label.back() == '0')
        {
            Label.pop_back();
        }
        if (!Label.empty() && Label.back() == '.')
        {
            Label.push_back('0');
        }
        return Label;
    }

    FString NormalizeFilterText(const FString& Text)
    {
        FString Normalized = Text;
        std::transform(
            Normalized.begin(),
            Normalized.end(),
            Normalized.begin(),
            [](unsigned char Character)
            {
                return static_cast<char>(std::tolower(Character));
            });
        return Normalized;
    }

    bool MatchesNormalizedFilter(const FString& NormalizedFilter, const FString& Candidate)
    {
        return NormalizedFilter.empty() || NormalizeFilterText(Candidate).find(NormalizedFilter) != FString::npos;
    }

    bool IsPlaybackRatePresetSelected(float CurrentRate, float PresetRate)
    {
        return std::fabs(std::fabs(CurrentRate) - PresetRate) <= 0.0001f;
    }

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

    bool IsAttributeCurveGroup(const FAnimationSequenceCurveViewGroup& Group)
    {
        return Group.CurveType == EAnimCurveType::Attribute;
    }

    TArray<FAnimationSequenceCurveViewGroup> PartitionCurveGroups(
        const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups,
        bool bAttributesOnly)
    {
        TArray<FAnimationSequenceCurveViewGroup> Result;
        for (const FAnimationSequenceCurveViewGroup& Group : CurveGroups)
        {
            if (IsAttributeCurveGroup(Group) == bAttributesOnly)
            {
                Result.push_back(Group);
            }
        }

        return Result;
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
}

void FAnimationSequenceEditorWidget::EnsurePlaybackControlIconsLoaded()
{
    if (PlaybackControlIcons.bAttemptedLoad)
    {
        return;
    }

    PlaybackControlIcons.bAttemptedLoad = true;
    FResourceManager& ResourceManager = FResourceManager::Get();
    PlaybackControlIcons.JumpToStart = ResourceManager.LoadTexture(GetPlaybackIconPathJumpToStart());
    PlaybackControlIcons.StepPrevious = ResourceManager.LoadTexture(GetPlaybackIconPathStepPrevious());
    PlaybackControlIcons.PlayReverse = ResourceManager.LoadTexture(GetPlaybackIconPathPlayReverse());
    PlaybackControlIcons.Record = ResourceManager.LoadTexture(GetPlaybackIconPathRecord());
    PlaybackControlIcons.PlayForward = ResourceManager.LoadTexture(GetPlaybackIconPathPlayForward());
    PlaybackControlIcons.Pause = ResourceManager.LoadTexture(GetPlaybackIconPathPause());
    PlaybackControlIcons.Stop = ResourceManager.LoadTexture(GetPlaybackIconPathStop());
    PlaybackControlIcons.StepNext = ResourceManager.LoadTexture(GetPlaybackIconPathStepNext());
    PlaybackControlIcons.JumpToEnd = ResourceManager.LoadTexture(GetPlaybackIconPathJumpToEnd());
    PlaybackControlIcons.LoopEnabled = ResourceManager.LoadTexture(GetPlaybackIconPathLoopEnabled());
    PlaybackControlIcons.LoopDisabled = ResourceManager.LoadTexture(GetPlaybackIconPathLoopDisabled());
}

bool FAnimationSequenceEditorWidget::DrawPlaybackControlButton(
    const char* ButtonId,
    UTexture* IconTexture,
    const char* FallbackLabel,
    const char* Tooltip,
    const ImVec2& Size,
    bool bSelected) const
{
    (void)bSelected;

    bool bPressed = false;
    if (IconTexture && IconTexture->GetSRV())
    {
        bPressed = ImGui::ImageButton(ButtonId, reinterpret_cast<void*>(IconTexture->GetSRV()), Size);
    }
    else
    {
        bPressed = ImGui::Button(FallbackLabel, Size);
    }

    if (ImGui::IsItemHovered() && Tooltip)
    {
        ImGui::SetTooltip("%s", Tooltip);
    }

    return bPressed;
}

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
    ImVec2 PreviewSize(ImGui::GetContentRegionAvail().x, PreviewHeight);
    PreviewSize.x = std::max(PreviewSize.x, 1.0f);
    PreviewSize.y = std::max(PreviewSize.y, 1.0f);

    ImGui::BeginChild("AnimSequencePreviewViewport", PreviewSize, true, ImGuiWindowFlags_NoScrollbar);
    if (PreviewController)
    {
        PreviewController->SetViewportSize(static_cast<int32>(PreviewSize.x), static_cast<int32>(PreviewSize.y));
    }

    if (PreviewController && PreviewController->HasValidPreview() && PreviewController->GetPreviewSRV())
    {
        RenderPreviewImage(PreviewSize, PreviewOverlayLines);
    }
    else
    {
        RenderPreviewFallback(PreviewSize, PreviewOverlayLines);
    }

    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderPreviewImage(
    const ImVec2& PreviewSize,
    const TArray<FString>& PreviewOverlayLines)
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

    SyncEmbeddedViewportRectAndFocus(Min, Max, bViewportClicked);
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
    bool bViewportClicked)
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

    const POINT ClientMin = ImGuiScreenToClientPoint(EditorEngine ? EditorEngine->GetWindow() : nullptr, Min);
    FViewportRect NewRect = {};
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

void FAnimationSequenceEditorWidget::RenderSequencerRegion(float SequencerHeight, float MaxPreviewHeight)
{
    const TArray<FAnimationSequenceCurveViewGroup> AllCurveGroups =
        EditorState ? AnimationSequenceCurveFilter::BuildCurveViewGroups(Sequence, *EditorState) : TArray<FAnimationSequenceCurveViewGroup>();
    const TArray<FAnimationSequenceCurveViewGroup> CurveGroups = PartitionCurveGroups(AllCurveGroups, false);
    const TArray<FAnimationSequenceCurveViewGroup> AttributeCurveGroups = PartitionCurveGroups(AllCurveGroups, true);

    ImDrawList* SplitterDrawList = ImGui::GetWindowDrawList();
    const float SplitterWidth = std::max(ImGui::GetContentRegionAvail().x, 1.0f);
    ImGui::InvisibleButton(
        "##AnimSequencePreviewSequencerSplitter",
        ImVec2(SplitterWidth, FAnimationSequenceSequencerLayout::SectionSplitterHeight));
    const bool bSplitterHovered = ImGui::IsItemHovered();
    const bool bSplitterActive = ImGui::IsItemActive();
    const ImVec2 SplitterMin = ImGui::GetItemRectMin();
    const ImVec2 SplitterMax = ImGui::GetItemRectMax();
    const ImU32 SplitterColor = ImGui::GetColorU32(
        bSplitterActive ? ImGuiCol_SeparatorActive : (bSplitterHovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator));
    SplitterDrawList->AddRectFilled(SplitterMin, SplitterMax, SplitterColor, 2.0f);
    if (bSplitterHovered || bSplitterActive)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
    if (bSplitterActive)
    {
        EditorState->PreviewPaneHeight = std::clamp(
            EditorState->PreviewPaneHeight + ImGui::GetIO().MouseDelta.y,
            FAnimationSequenceSequencerLayout::PreviewMinHeight,
            MaxPreviewHeight);
    }

    ImGui::BeginChild("##AnimationSequenceSequencerRegion", ImVec2(0.0f, SequencerHeight), true);
    const float DefaultDetailsPaneHeight =
        std::min(FAnimationSequenceSequencerLayout::DetailsPanelHeight, SequencerHeight * 0.34f);
    const float MaxDetailsPaneHeight = std::max(
        FAnimationSequenceSequencerLayout::DetailsPanelMinHeight,
        SequencerHeight -
            FAnimationSequenceSequencerLayout::BottomControlStripHeight -
            FAnimationSequenceSequencerLayout::SectionSplitterHeight -
            140.0f);
    if (EditorState && EditorState->SequencerDetailsPaneHeight <= 0.0f)
    {
        EditorState->SequencerDetailsPaneHeight = DefaultDetailsPaneHeight;
    }
    const float DetailsPaneHeight = EditorState
        ? std::clamp(
            EditorState->SequencerDetailsPaneHeight,
            FAnimationSequenceSequencerLayout::DetailsPanelMinHeight,
            MaxDetailsPaneHeight)
        : DefaultDetailsPaneHeight;
    const float SplitPaneHeight = std::max(
        SequencerHeight -
        DetailsPaneHeight -
        FAnimationSequenceSequencerLayout::SectionSplitterHeight,
        140.0f);
    if (EditorState)
    {
        EditorState->SequencerDetailsPaneHeight = DetailsPaneHeight;
    }

    const float MaxOutlinerWidth = std::max(
        FAnimationSequenceSequencerLayout::OutlinerMinWidth,
        ImGui::GetContentRegionAvail().x - 280.0f);
    RenderSequencerSplitPane(
        SplitPaneHeight,
        MaxOutlinerWidth,
        CurveGroups,
        AttributeCurveGroups);

    ImGui::InvisibleButton(
        "##AnimSequenceTracksDetailsSplitter",
        ImVec2(std::max(ImGui::GetContentRegionAvail().x, 1.0f), FAnimationSequenceSequencerLayout::SectionSplitterHeight));
    const bool bDetailsSplitterHovered = ImGui::IsItemHovered();
    const bool bDetailsSplitterActive = ImGui::IsItemActive();
    const ImVec2 DetailsSplitterMin = ImGui::GetItemRectMin();
    const ImVec2 DetailsSplitterMax = ImGui::GetItemRectMax();
    const ImU32 DetailsSplitterColor = ImGui::GetColorU32(
        bDetailsSplitterActive ? ImGuiCol_SeparatorActive : (bDetailsSplitterHovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator));
    SplitterDrawList->AddRectFilled(DetailsSplitterMin, DetailsSplitterMax, DetailsSplitterColor, 2.0f);
    if (bDetailsSplitterHovered || bDetailsSplitterActive)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
    if (bDetailsSplitterActive && EditorState)
    {
        EditorState->SequencerDetailsPaneHeight = std::clamp(
            EditorState->SequencerDetailsPaneHeight - ImGui::GetIO().MouseDelta.y,
            FAnimationSequenceSequencerLayout::DetailsPanelMinHeight,
            MaxDetailsPaneHeight);
    }

    RenderSelectionDetailsPane(DetailsPaneHeight);
    ImGui::EndChild();
}

FAnimationSequenceSequencerVisibleLayout FAnimationSequenceEditorWidget::BuildSequencerVisibleLayout(
    const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups,
    const TArray<FAnimationSequenceCurveViewGroup>& AttributeCurveGroups) const
{
    FAnimationSequenceSequencerVisibleLayout Layout;
    if (!EditorState)
    {
        return Layout;
    }

    float CursorY = 0.0f;
    auto AddRow = [&](EAnimationSequenceSequencerVisibleRowType Type, float Height) -> FAnimationSequenceSequencerVisibleRow&
    {
        FAnimationSequenceSequencerVisibleRow& Row = Layout.Rows.emplace_back();
        Row.Type = Type;
        Row.Id = MakeAnimationSequenceSequencerRowId(Type);
        Row.OffsetY = CursorY;
        Row.Height = Height;
        CursorY += Height;
        return Row;
    };

    const FString NormalizedFilter = NormalizeFilterText(TrackFilterEditBuffer.data());

    FAnimationSequenceSequencerVisibleRow& NotifyHeader =
        AddRow(EAnimationSequenceSequencerVisibleRowType::NotifyHeader, FAnimationSequenceSequencerLayout::SectionHeaderHeight);
    NotifyHeader.Id = MakeAnimationSequenceSequencerRowId(EAnimationSequenceSequencerVisibleRowType::NotifyHeader);
    NotifyHeader.Label = "Notifies";
    const int32 NotifyTrackCount = Document ? Document->GetNotifyTrackCount() : 0;
    NotifyHeader.bCanExpand = NotifyTrackCount > 0;
    NotifyHeader.bExpanded = EditorState->bNotifiesExpanded;
    if (NotifyHeader.bCanExpand && EditorState->bNotifiesExpanded)
    {
        for (int32 TrackIndex = 0; TrackIndex < NotifyTrackCount; ++TrackIndex)
        {
            const FAnimNotifyTrack* Track = Document ? Document->GetNotifyTrack(TrackIndex) : nullptr;
            const FString TrackLabel = Track && Track->TrackName.IsValid()
                ? Track->TrackName.ToString()
                : ("Track " + std::to_string(TrackIndex + 1));
            if (!MatchesNormalizedFilter(NormalizedFilter, TrackLabel))
            {
                continue;
            }

            FAnimationSequenceSequencerVisibleRow& Row =
                AddRow(EAnimationSequenceSequencerVisibleRowType::NotifyTrack, FAnimationSequenceSequencerLayout::NotifyTrackRowHeight);
            Row.SourceIndex = TrackIndex;
            Row.Id = MakeAnimationSequenceSequencerRowId(EAnimationSequenceSequencerVisibleRowType::NotifyTrack, TrackIndex);
            Row.Label = TrackLabel;
            Row.SecondaryLabel = Track ? std::to_string(static_cast<int32>(Track->Events.size())) : FString("0");
            Row.bSelected = EditorState->SelectedNotifyTrackIndex == TrackIndex;
        }
    }

    FAnimationSequenceSequencerVisibleRow& CurveHeader =
        AddRow(EAnimationSequenceSequencerVisibleRowType::CurveHeader, FAnimationSequenceSequencerLayout::SectionHeaderHeight);
    CurveHeader.Id = MakeAnimationSequenceSequencerRowId(EAnimationSequenceSequencerVisibleRowType::CurveHeader);
    CurveHeader.Label = "Curves";
    CurveHeader.bCanExpand = !CurveGroups.empty();
    CurveHeader.bExpanded = EditorState->bCurvesExpanded;
    if (CurveHeader.bCanExpand && EditorState->bCurvesExpanded)
    {
        for (int32 GroupIndex = 0; GroupIndex < static_cast<int32>(CurveGroups.size()); ++GroupIndex)
        {
            const FAnimationSequenceCurveViewGroup& Group = CurveGroups[GroupIndex];
            const bool bGroupMatchesFilter = MatchesNormalizedFilter(NormalizedFilter, Group.Label);
            TArray<FAnimationSequenceCurveViewEntry> MatchingEntries;
            MatchingEntries.reserve(Group.VisibleEntries.size());
            for (const FAnimationSequenceCurveViewEntry& Entry : Group.VisibleEntries)
            {
                const FString CurveLabel = Entry.Curve ? Entry.Curve->CurveName.ToString() : FString("Curve");
                if (MatchesNormalizedFilter(NormalizedFilter, CurveLabel))
                {
                    MatchingEntries.push_back(Entry);
                }
            }

            if (!NormalizedFilter.empty() && !bGroupMatchesFilter && MatchingEntries.empty())
            {
                continue;
            }

            FAnimationSequenceSequencerVisibleRow& GroupRow =
                AddRow(EAnimationSequenceSequencerVisibleRowType::CurveGroup, FAnimationSequenceSequencerLayout::CurveGroupHeaderHeight);
            GroupRow.GroupIndex = GroupIndex;
            GroupRow.Id = MakeAnimationSequenceSequencerRowId(EAnimationSequenceSequencerVisibleRowType::CurveGroup, -1, GroupIndex);
            GroupRow.Label = Group.Label;
            GroupRow.SecondaryLabel = std::to_string(Group.TotalCount);
            GroupRow.bToggleChecked = Group.bVisible;
            GroupRow.CurveType = Group.CurveType;

            if (Group.bVisible && !MatchingEntries.empty())
            {
                for (int32 EntryIndex = 0; EntryIndex < static_cast<int32>(MatchingEntries.size()); ++EntryIndex)
                {
                    const FAnimationSequenceCurveViewEntry& Entry = MatchingEntries[EntryIndex];
                    FAnimationSequenceSequencerVisibleRow& Row =
                        AddRow(EAnimationSequenceSequencerVisibleRowType::CurveEntry, FAnimationSequenceSequencerLayout::CurveTrackRowHeight);
                    Row.SourceIndex = Entry.SourceIndex;
                    Row.Id = MakeAnimationSequenceSequencerRowId(EAnimationSequenceSequencerVisibleRowType::CurveEntry, Entry.SourceIndex, GroupIndex);
                    Row.Curve = Entry.Curve;
                    Row.CurveType = Group.CurveType;
                    Row.Label = Entry.Curve ? Entry.Curve->CurveName.ToString() : FString("Curve");
                    Row.SecondaryLabel = Entry.Curve
                        ? (std::to_string(static_cast<int32>(Entry.Curve->Keys.size())) + " keys")
                        : FString();
                    Row.bSelected = EditorState->SelectedCurveIndex == Entry.SourceIndex;
                }
            }
        }
    }

    FAnimationSequenceSequencerVisibleRow& AdditiveHeader =
        AddRow(EAnimationSequenceSequencerVisibleRowType::AdditiveHeader, FAnimationSequenceSequencerLayout::SectionHeaderHeight);
    AdditiveHeader.Id = MakeAnimationSequenceSequencerRowId(EAnimationSequenceSequencerVisibleRowType::AdditiveHeader);
    AdditiveHeader.Label = "Additive Layer Tracks";
    AdditiveHeader.bCanExpand = false;
    AdditiveHeader.bExpanded = EditorState->bAdditiveLayerTracksExpanded;
    if (AdditiveHeader.bCanExpand && EditorState->bAdditiveLayerTracksExpanded)
    {
        FAnimationSequenceSequencerVisibleRow& Row =
            AddRow(EAnimationSequenceSequencerVisibleRowType::AdditiveEmpty, FAnimationSequenceSequencerLayout::EmptySectionRowHeight);
        Row.Id = MakeAnimationSequenceSequencerRowId(EAnimationSequenceSequencerVisibleRowType::AdditiveEmpty);
        Row.Label = "No additive layer tracks.";
    }

    FAnimationSequenceSequencerVisibleRow& AttributeHeader =
        AddRow(EAnimationSequenceSequencerVisibleRowType::AttributeHeader, FAnimationSequenceSequencerLayout::SectionHeaderHeight);
    AttributeHeader.Id = MakeAnimationSequenceSequencerRowId(EAnimationSequenceSequencerVisibleRowType::AttributeHeader);
    AttributeHeader.Label = "Attributes";
    AttributeHeader.bCanExpand = !AttributeCurveGroups.empty();
    AttributeHeader.bExpanded = EditorState->bAttributesExpanded;
    if (AttributeHeader.bCanExpand && EditorState->bAttributesExpanded)
    {
        for (int32 GroupIndex = 0; GroupIndex < static_cast<int32>(AttributeCurveGroups.size()); ++GroupIndex)
        {
            const FAnimationSequenceCurveViewGroup& Group = AttributeCurveGroups[GroupIndex];
            const bool bGroupMatchesFilter = MatchesNormalizedFilter(NormalizedFilter, Group.Label);
            TArray<FAnimationSequenceCurveViewEntry> MatchingEntries;
            MatchingEntries.reserve(Group.VisibleEntries.size());
            for (const FAnimationSequenceCurveViewEntry& Entry : Group.VisibleEntries)
            {
                const FString CurveLabel = Entry.Curve ? Entry.Curve->CurveName.ToString() : FString("Curve");
                if (MatchesNormalizedFilter(NormalizedFilter, CurveLabel))
                {
                    MatchingEntries.push_back(Entry);
                }
            }

            if (!NormalizedFilter.empty() && !bGroupMatchesFilter && MatchingEntries.empty())
            {
                continue;
            }

            FAnimationSequenceSequencerVisibleRow& GroupRow =
                AddRow(EAnimationSequenceSequencerVisibleRowType::AttributeGroup, FAnimationSequenceSequencerLayout::CurveGroupHeaderHeight);
            GroupRow.GroupIndex = GroupIndex;
            GroupRow.Id = MakeAnimationSequenceSequencerRowId(EAnimationSequenceSequencerVisibleRowType::AttributeGroup, -1, GroupIndex);
            GroupRow.Label = Group.Label;
            GroupRow.SecondaryLabel = std::to_string(Group.TotalCount);
            GroupRow.bToggleChecked = Group.bVisible;
            GroupRow.CurveType = Group.CurveType;

            if (Group.bVisible && !MatchingEntries.empty())
            {
                for (int32 EntryIndex = 0; EntryIndex < static_cast<int32>(MatchingEntries.size()); ++EntryIndex)
                {
                    const FAnimationSequenceCurveViewEntry& Entry = MatchingEntries[EntryIndex];
                    FAnimationSequenceSequencerVisibleRow& Row =
                        AddRow(EAnimationSequenceSequencerVisibleRowType::AttributeEntry, FAnimationSequenceSequencerLayout::CurveTrackRowHeight);
                    Row.SourceIndex = Entry.SourceIndex;
                    Row.Id = MakeAnimationSequenceSequencerRowId(EAnimationSequenceSequencerVisibleRowType::AttributeEntry, Entry.SourceIndex, GroupIndex);
                    Row.Curve = Entry.Curve;
                    Row.CurveType = Group.CurveType;
                    Row.Label = Entry.Curve ? Entry.Curve->CurveName.ToString() : FString("Curve");
                    Row.SecondaryLabel = Entry.Curve
                        ? (std::to_string(static_cast<int32>(Entry.Curve->Keys.size())) + " keys")
                        : FString();
                    Row.bSelected = EditorState->SelectedCurveIndex == Entry.SourceIndex;
                }
            }
        }
    }

    Layout.ContentHeight = CursorY;
    return Layout;
}

void FAnimationSequenceEditorWidget::RenderSequencerSplitPane(
    float SplitPaneHeight,
    float MaxOutlinerWidth,
    const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups,
    const TArray<FAnimationSequenceCurveViewGroup>& AttributeCurveGroups)
{
    EditorState->TrackOutlinerWidth = std::clamp(
        EditorState->TrackOutlinerWidth,
        FAnimationSequenceSequencerLayout::OutlinerMinWidth,
        MaxOutlinerWidth);

    const FAnimationSequenceSequencerVisibleLayout VisibleLayout =
        BuildSequencerVisibleLayout(CurveGroups, AttributeCurveGroups);
    bool bFocusedRowVisible = !EditorState->FocusedSequencerRowId.IsValid();
    for (const FAnimationSequenceSequencerVisibleRow& Row : VisibleLayout.Rows)
    {
        if (EditorState->FocusedSequencerRowId.IsValid() && Row.Id == EditorState->FocusedSequencerRowId)
        {
            bFocusedRowVisible = true;
            break;
        }
    }
    if (!bFocusedRowVisible)
    {
        EditorState->ClearFocusedSequencerRow();
    }
    if (!EditorState->FocusedSequencerRowId.IsValid())
    {
        for (const FAnimationSequenceSequencerVisibleRow& Row : VisibleLayout.Rows)
        {
            if (Row.bSelected)
            {
                EditorState->SetFocusedSequencerRow(Row.Id);
                break;
            }
        }
    }
    EditorState->ClearHoveredSequencerRow();
    const float TrackBodyHeight = VisibleLayout.ContentHeight;
    const float MainPaneHeight = std::max(
        SplitPaneHeight -
            FAnimationSequenceSequencerLayout::BottomControlStripHeight -
            SequencerHeaderPadding,
        120.0f);
    const float TrackListHeight = std::max(
        MainPaneHeight -
            GetTrackOutlinerFilterHeight() -
            SequencerHeaderPadding,
        80.0f);
    const float TimelineCanvasHeight = std::max(MainPaneHeight, 80.0f);
    const float TrackViewportHeight =
        std::min(TrackListHeight, TimelineCanvasHeight) -
        FAnimationSequenceSequencerLayout::RulerHeight -
        FAnimationSequenceTimelineGeometry::VerticalPadding * 2.0f;
    const float MaxScrollY = std::max(
        0.0f,
        TrackBodyHeight + FAnimationSequenceSequencerLayout::TrackAreaPadding * 2.0f - TrackViewportHeight);
    EditorState->SequencerScrollY = std::clamp(EditorState->SequencerScrollY, 0.0f, MaxScrollY);

    ImGui::BeginChild("##AnimationSequenceSequencerSplit", ImVec2(0.0f, SplitPaneHeight), false, ImGuiWindowFlags_NoScrollbar);
    const float SplitWidth = std::max(ImGui::GetContentRegionAvail().x, 1.0f);
    const float TimelinePaneWidth = std::max(
        SplitWidth -
            EditorState->TrackOutlinerWidth -
            FAnimationSequenceSequencerLayout::OutlinerSplitterWidth,
        120.0f);
    ImGui::BeginChild("##AnimationSequenceSequencerMainRow", ImVec2(0.0f, MainPaneHeight), false, ImGuiWindowFlags_NoScrollbar);
    RenderTrackOutlinerPane(EditorState->TrackOutlinerWidth, MainPaneHeight, VisibleLayout);

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::InvisibleButton(
        "##AnimationSequenceOutlinerSplitter",
        ImVec2(FAnimationSequenceSequencerLayout::OutlinerSplitterWidth, MainPaneHeight));
    const bool bHovered = ImGui::IsItemHovered();
    const bool bActive = ImGui::IsItemActive();
    const ImVec2 SplitterMin = ImGui::GetItemRectMin();
    const ImVec2 SplitterMax = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddRectFilled(
        SplitterMin,
        SplitterMax,
        ImGui::GetColorU32(bActive ? ImGuiCol_SeparatorActive : (bHovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator)),
        2.0f);
    if (bHovered || bActive)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    if (bActive)
    {
        EditorState->TrackOutlinerWidth = std::clamp(
            EditorState->TrackOutlinerWidth + ImGui::GetIO().MouseDelta.x,
            FAnimationSequenceSequencerLayout::OutlinerMinWidth,
            MaxOutlinerWidth);
    }

    ImGui::SameLine(0.0f, 0.0f);
    RenderTimelineCanvasPane(
        TimelinePaneWidth,
        MainPaneHeight,
        VisibleLayout);

    ImGui::EndChild();
    RenderBottomControlStrip(
        FAnimationSequenceSequencerLayout::BottomControlStripHeight,
        EditorState->TrackOutlinerWidth,
        TimelinePaneWidth);
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderTrackOutlinerPane(
    float Width,
    float Height,
    const FAnimationSequenceSequencerVisibleLayout& VisibleLayout)
{
    const float FilterHeight = GetTrackOutlinerFilterHeight();
    const float TrackListHeight = std::max(Height - FilterHeight - SequencerHeaderPadding, 80.0f);

    ImGui::BeginChild(
        "##AnimationSequenceTrackOutlinerPane",
        ImVec2(Width, Height),
        true);

    const float FilterBandTopY = ImGui::GetCursorPosY();
    const float FilterInputHeight = ImGui::GetFrameHeight();
    ImGui::SetCursorPosY(
        FilterBandTopY +
        std::max(0.0f, (FilterHeight - FilterInputHeight) * 0.5f));
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint(
        "##AnimationSequenceTrackFilter",
        "Filter tracks and curves",
        TrackFilterEditBuffer.data(),
        TrackFilterEditBuffer.size());
    ImGui::SetCursorPosY(FilterBandTopY + FilterHeight);

    ImGui::BeginChild(
        "##AnimationSequenceTrackOutlinerList",
        ImVec2(0.0f, TrackListHeight),
        false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    const float RowOriginY = ImGui::GetCursorScreenPos().y - EditorState->SequencerScrollY;
    TrackOutlinerWidget.Render(*EditorState, Document, VisibleLayout, RowOriginY);
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
        std::fabs(ImGui::GetIO().MouseWheel) > 0.0f)
    {
        EditorState->SequencerScrollY = std::max(EditorState->SequencerScrollY - ImGui::GetIO().MouseWheel * 24.0f, 0.0f);
    }
    ImGui::EndChild();

    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderTimelineCanvasPane(
    float Width,
    float Height,
    const FAnimationSequenceSequencerVisibleLayout& VisibleLayout)
{
    ImGui::BeginChild(
        "##AnimationSequenceTimelineCanvasPane",
        ImVec2(Width, Height),
        true);

    ImGui::BeginChild(
        "##AnimationSequenceTimelineCanvasViewport",
        ImVec2(0.0f, 0.0f),
        false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const float RequiredCanvasHeight =
        FAnimationSequenceTimelineGeometry::VerticalPadding * 2.0f +
        TimelineWidget.GetRulerHeight() +
        FAnimationSequenceSequencerLayout::TrackAreaPadding * 2.0f +
        VisibleLayout.ContentHeight;

    const ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
    const ImVec2 CanvasSize(
        std::max(1.0f, ImGui::GetContentRegionAvail().x),
        std::max(std::max(Height - 2.0f, 1.0f), RequiredCanvasHeight));
    ImGui::Dummy(CanvasSize);

    const ImVec2 CanvasEnd(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y);
    const bool bCanvasHovered =
        ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
        ImGui::IsMouseHoveringRect(CanvasPos, CanvasEnd, false);
    const bool bCanvasActive =
        bCanvasHovered &&
        (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Middle));
    const FAnimationSequenceTimelineGeometry Geometry =
        FAnimationSequenceTimelineGeometry::BuildSequencerCanvasGeometry(CanvasPos, CanvasSize, TimelineWidget.GetRulerHeight());
    const float TimelineRowOriginY =
        Geometry.TrackTop +
        FAnimationSequenceSequencerLayout::TrackAreaPadding -
        EditorState->SequencerScrollY;
    if (bCanvasHovered)
    {
        const ImVec2 MousePos = ImGui::GetIO().MousePos;
        for (const FAnimationSequenceSequencerVisibleRow& Row : VisibleLayout.Rows)
        {
            const float RowTop = TimelineRowOriginY + Row.OffsetY;
            const float RowBottom = RowTop + Row.Height;
            if (MousePos.x >= Geometry.TimelineMinX &&
                MousePos.x <= Geometry.TimelineMaxX &&
                MousePos.y >= RowTop &&
                MousePos.y <= RowBottom)
            {
                EditorState->SetHoveredSequencerRow(Row.Id);
                break;
            }
        }
    }

    TimelineWidget.RenderCanvas(*EditorState, PreviewController, Geometry, bCanvasHovered, bCanvasActive);
    DrawTimelineRowBackgrounds(Geometry, VisibleLayout, TimelineRowOriginY);
    NotifyLaneWidget.RenderRows(*EditorState, Document, Geometry, VisibleLayout, TimelineRowOriginY);
    EditorState->HoveredCurveIndex = -1;
    EditorState->bHasHoveredCurveSample = false;
    EditorState->HoveredCurveSampleTime = 0.0f;
    EditorState->HoveredCurveSampleValue = 0.0f;
    EditorState->HoveredCurveNearestKeyIndex = -1;
    CurveTrackWidget.RenderRows(*EditorState, Geometry, VisibleLayout, TimelineRowOriginY);
    if (Document &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        EditorState->HoveredCurveIndex >= 0)
    {
        Document->ClearNotifySelection();
    }

    ImGui::EndChild();
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::DrawTimelineRowBackgrounds(
    const FAnimationSequenceTimelineGeometry& Geometry,
    const FAnimationSequenceSequencerVisibleLayout& VisibleLayout,
    float TimelineRowOriginY) const
{
    const float ClipBottom = Geometry.CanvasEnd.y - FAnimationSequenceTimelineGeometry::VerticalPadding;
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    const ImU32 SeparatorColor = IM_COL32(255, 255, 255, 18);
    const ImU32 HeaderSeparatorColor = IM_COL32(255, 255, 255, 26);
    const ImU32 HeaderFillColor = IM_COL32(255, 255, 255, 10);
    const ImU32 HoverFillColor = IM_COL32(255, 255, 255, 14);
    const ImU32 FocusFillColor = IM_COL32(78, 116, 168, 64);

    DrawList->PushClipRect(
        ImVec2(Geometry.TimelineMinX, Geometry.TrackTop),
        ImVec2(Geometry.TimelineMaxX, ClipBottom),
        true);

    bool bDrewFirstBoundary = false;
    for (const FAnimationSequenceSequencerVisibleRow& Row : VisibleLayout.Rows)
    {
        const float RowTop = TimelineRowOriginY + Row.OffsetY;
        const float RowBottom = RowTop + Row.Height;
        if (RowBottom < Geometry.TrackTop || RowTop > ClipBottom)
        {
            continue;
        }

        const bool bHovered = EditorState && EditorState->IsHoveredSequencerRow(Row.Id);
        const bool bFocused = EditorState && EditorState->IsFocusedSequencerRow(Row.Id);
        const bool bHeader = IsAnimationSequenceSectionHeaderRow(Row.Type);
        ImU32 FillColor = 0;
        if (bFocused)
        {
            FillColor = FocusFillColor;
        }
        else if (bHovered)
        {
            FillColor = HoverFillColor;
        }
        else if (bHeader)
        {
            FillColor = HeaderFillColor;
        }

        if (FillColor != 0)
        {
            DrawList->AddRectFilled(
                ImVec2(Geometry.TimelineMinX, RowTop),
                ImVec2(Geometry.TimelineMaxX, RowBottom),
                FillColor);
        }

        const ImU32 TopColor = IsAnimationSequenceSectionHeaderRow(Row.Type) ? HeaderSeparatorColor : SeparatorColor;
        if (!bDrewFirstBoundary)
        {
            DrawList->AddLine(
                ImVec2(Geometry.TimelineMinX, RowTop),
                ImVec2(Geometry.TimelineMaxX, RowTop),
                TopColor);
            bDrewFirstBoundary = true;
        }

        DrawList->AddLine(
            ImVec2(Geometry.TimelineMinX, RowBottom),
            ImVec2(Geometry.TimelineMaxX, RowBottom),
            TopColor);
    }

    DrawList->PopClipRect();
}

void FAnimationSequenceEditorWidget::RenderBottomControlStrip(float Height, float LeftPaneWidth, float RightPaneWidth)
{
    const bool bCanTimelineControl =
        PreviewController != nullptr && EditorState != nullptr && EditorState->HasTimelineData();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::BeginChild(
        "##AnimationSequenceBottomControlStrip",
        ImVec2(0.0f, Height),
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::BeginChild(
        "##AnimationSequenceBottomTransportPane",
        ImVec2(LeftPaneWidth, Height),
        false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    RenderTransportControls(
        bCanTimelineControl,
        PreviewController != nullptr && PreviewController->HasValidPreview(),
        Height);
    ImGui::EndChild();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginChild(
        "##AnimationSequenceBottomTransportSpacer",
        ImVec2(FAnimationSequenceSequencerLayout::OutlinerSplitterWidth, Height),
        false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::EndChild();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginChild(
        "##AnimationSequenceBottomTimelinePane",
        ImVec2(RightPaneWidth, Height),
        false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    RenderTimelineRangeControls(Height);
    ImGui::EndChild();

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void FAnimationSequenceEditorWidget::RenderTimelineRangeControls(float StripHeight)
{
    (void)StripHeight;
    const FTransportStripVerticalCenter VerticalCenter
    {
        0.0f,
        std::max(ImGui::GetWindowHeight(), ImGui::GetContentRegionAvail().y)
    };
    const float SliderHeight = 12.0f;
    if (!EditorState || !EditorState->HasTimelineData())
    {
        VerticalCenter.AlignCursorY(ImGui::GetTextLineHeight());
        ImGui::TextDisabled("Timeline view controls are unavailable.");
        return;
    }

    const float MaximumTime = std::max(EditorState->GetMaxTime(), EditorState->GetMinimumVisibleRange());
    const float SequenceStart = EditorState->GetTimelineRangeStart();
    const float SequenceEnd = EditorState->GetTimelineRangeEnd();
    const float PlaybackStart = EditorState->GetPlaybackRangeStart();
    const float PlaybackEnd = EditorState->GetPlaybackRangeEnd();

    const FString ViewStartLabel = FormatCompactTimelineRangeLabel(*EditorState, EditorState->VisibleTimeStart);
    const FString PlaybackStartLabel = FormatCompactTimelineRangeLabel(*EditorState, PlaybackStart);
    const FString SequenceStartLabel = FormatCompactTimelineRangeLabel(*EditorState, SequenceStart);
    const FString SequenceEndLabel = FormatCompactTimelineRangeLabel(*EditorState, SequenceEnd);
    const FString PlaybackEndLabel = FormatCompactTimelineRangeLabel(*EditorState, PlaybackEnd);
    const FString ViewEndLabel = FormatCompactTimelineRangeLabel(*EditorState, EditorState->VisibleTimeEnd);

    struct FRangeLabelSpec
    {
        FString Text;
        ImVec4 Color;
        const char* Tooltip = nullptr;
    };

    const ImVec4 NeutralColor(0.62f, 0.66f, 0.74f, 1.0f);
    const ImVec4 PlaybackStartColor(0.37f, 0.78f, 0.42f, 1.0f);
    const ImVec4 PlaybackEndColor(0.90f, 0.30f, 0.28f, 1.0f);
    const TArray<FRangeLabelSpec> LeftLabels =
    {
        { ViewStartLabel, NeutralColor, "View start" },
        { PlaybackStartLabel, PlaybackStartColor, "Playback start" },
        { SequenceStartLabel, NeutralColor, "Sequence start" }
    };
    const TArray<FRangeLabelSpec> RightLabels =
    {
        { SequenceEndLabel, NeutralColor, "Sequence end" },
        { PlaybackEndLabel, PlaybackEndColor, "Playback end" },
        { ViewEndLabel, NeutralColor, "View end" }
    };

    constexpr float EdgePadding = 32.0f;
    constexpr float LabelSpacing = 12.0f;
    constexpr float GroupSpacing = 20.0f;
    auto MeasureLabelsWidth = [&](const TArray<FRangeLabelSpec>& Labels)
    {
        float Width = 0.0f;
        for (int32 Index = 0; Index < static_cast<int32>(Labels.size()); ++Index)
        {
            Width += ImGui::CalcTextSize(Labels[Index].Text.c_str()).x;
            if (Index + 1 < static_cast<int32>(Labels.size()))
            {
                Width += LabelSpacing;
            }
        }
        return Width;
    };

    const float LeftLabelsWidth = MeasureLabelsWidth(LeftLabels);
    const float RightLabelsWidth = MeasureLabelsWidth(RightLabels);
    const float AvailableWidth = std::max(ImGui::GetContentRegionAvail().x, 1.0f);
    const float ReservedWidth =
        LeftLabelsWidth +
        RightLabelsWidth +
        EdgePadding * 2.0f +
        GroupSpacing * 2.0f;
    const float SliderWidth = std::max(36.0f, AvailableWidth - ReservedWidth);

    auto DrawRangeLabel = [&](const char* PopupId, const FRangeLabelSpec& LabelSpec, bool bEditable, auto&& ApplyEditedTime, float CurrentValue)
    {
        VerticalCenter.AlignCursorY(ImGui::GetTextLineHeight());
        ImGui::TextColored(LabelSpec.Color, "%s", LabelSpec.Text.c_str());
        if (bEditable && ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            ImGui::OpenPopup(PopupId);
        }
        if (ImGui::IsItemHovered() && LabelSpec.Tooltip)
        {
            ImGui::SetTooltip("%s%s", LabelSpec.Tooltip, bEditable ? " (Click to edit)" : "");
        }

        if (ImGui::BeginPopup(PopupId))
        {
            const bool bFirstPopupFrame = ActiveTimelineRangeEditPopupId != PopupId;
            if (bFirstPopupFrame)
            {
                ActiveTimelineRangeEditPopupId = PopupId;
                if (EditorState->TotalFrames > 0)
                {
                    CopyStringToBuffer(std::to_string(EditorState->TimeToFrameIndex(CurrentValue)), TimelineRangeEditBuffer);
                }
                else
                {
                    char SecondsBuffer[32];
                    snprintf(SecondsBuffer, sizeof(SecondsBuffer), "%.3f", CurrentValue);
                    CopyStringToBuffer(SecondsBuffer, TimelineRangeEditBuffer);
                }
                ImGui::SetKeyboardFocusHere();
            }

            const bool bPressedEnter =
                ImGui::InputText(
                    EditorState->TotalFrames > 0 ? "Frame" : "Seconds",
                    TimelineRangeEditBuffer.data(),
                    TimelineRangeEditBuffer.size(),
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

            bool bApply = bPressedEnter;

            ImGui::Spacing();
            if (ImGui::Button("Apply"))
            {
                bApply = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                ActiveTimelineRangeEditPopupId.clear();
                TimelineRangeEditBuffer.fill('\0');
                ImGui::CloseCurrentPopup();
            }

            if (bApply)
            {
                if (EditorState->TotalFrames > 0)
                {
                    ApplyEditedTime(EditorState->FrameToTime(std::atoi(TimelineRangeEditBuffer.data())));
                }
                else
                {
                    ApplyEditedTime(static_cast<float>(std::atof(TimelineRangeEditBuffer.data())));
                }

                ActiveTimelineRangeEditPopupId.clear();
                TimelineRangeEditBuffer.fill('\0');
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        else if (ActiveTimelineRangeEditPopupId == PopupId)
        {
            ActiveTimelineRangeEditPopupId.clear();
            TimelineRangeEditBuffer.fill('\0');
        }
    };

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + EdgePadding);
    DrawRangeLabel(
        "##AnimSequenceViewStartEdit",
        LeftLabels[0],
        true,
        [&](float NewTime) { EditorState->SetVisibleRange(NewTime, EditorState->VisibleTimeEnd); },
        EditorState->VisibleTimeStart);
    ImGui::SameLine(0.0f, LabelSpacing);
    DrawRangeLabel(
        "##AnimSequencePlaybackStartDisplay",
        LeftLabels[1],
        true,
        [&](float NewTime)
        {
            if (PreviewController)
            {
                PreviewController->SetPlaybackRange(NewTime, EditorState->GetPlaybackRangeEnd());
                EditorState->SetPlaybackRange(
                    PreviewController->GetPlaybackRangeStart(),
                    PreviewController->GetPlaybackRangeEnd());
            }
        },
        PlaybackStart);
    ImGui::SameLine(0.0f, LabelSpacing);
    DrawRangeLabel(
        "##AnimSequenceSequenceStartEdit",
        LeftLabels[2],
        true,
        [&](float NewTime) { EditorState->SetTimelineRange(NewTime, EditorState->GetTimelineRangeEnd()); },
        SequenceStart);

    ImGui::SameLine(0.0f, GroupSpacing);
    const float SliderStartX = ImGui::GetCursorPosX();
    VerticalCenter.AlignCursorY(SliderHeight);
    ImGui::InvisibleButton(
        "##AnimationSequenceTimelineViewRangeBar",
        ImVec2(SliderWidth, SliderHeight),
        ImGuiButtonFlags_MouseButtonLeft);

    enum class ERangeBarDragMode : uint8
    {
        None,
        StartHandle,
        EndHandle,
        Thumb
    };

    static ERangeBarDragMode ActiveDragMode = ERangeBarDragMode::None;
    static float DragMouseStartX = 0.0f;
    static float DragVisibleStart = 0.0f;
    static float DragVisibleEnd = 0.0f;

    const ImVec2 SliderMin = ImGui::GetItemRectMin();
    const ImVec2 SliderMax = ImGui::GetItemRectMax();
    const float SliderPixelWidth = std::max(SliderMax.x - SliderMin.x, 1.0f);
    const float HandleHalfWidth = 32.0f;

    auto TimeToSliderX = [&](float TimeValue)
    {
        const float Alpha = MaximumTime > 0.0f ? std::clamp(TimeValue / MaximumTime, 0.0f, 1.0f) : 0.0f;
        return SliderMin.x + SliderPixelWidth * Alpha;
    };

    float VisibleStart = EditorState->VisibleTimeStart;
    float VisibleEnd = EditorState->VisibleTimeEnd;
    const float ThumbMinX = TimeToSliderX(VisibleStart);
    const float ThumbMaxX = TimeToSliderX(VisibleEnd);

    if (ImGui::IsItemActivated())
    {
        DragMouseStartX = ImGui::GetIO().MousePos.x;
        DragVisibleStart = VisibleStart;
        DragVisibleEnd = VisibleEnd;

        const float MouseX = ImGui::GetIO().MousePos.x;
        const bool bNearStartHandle = std::fabs(MouseX - ThumbMinX) <= HandleHalfWidth + 2.0f;
        const bool bNearEndHandle = std::fabs(MouseX - ThumbMaxX) <= HandleHalfWidth + 2.0f;
        const bool bInsideThumb = MouseX >= ThumbMinX && MouseX <= ThumbMaxX;

        if (bNearStartHandle)
        {
            ActiveDragMode = ERangeBarDragMode::StartHandle;
        }
        else if (bNearEndHandle)
        {
            ActiveDragMode = ERangeBarDragMode::EndHandle;
        }
        else if (bInsideThumb)
        {
            ActiveDragMode = ERangeBarDragMode::Thumb;
        }
        else
        {
            ActiveDragMode = MouseX < ThumbMinX ? ERangeBarDragMode::StartHandle : ERangeBarDragMode::EndHandle;
        }
    }

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        const float DeltaPixels = ImGui::GetIO().MousePos.x - DragMouseStartX;
        const float DeltaTime = MaximumTime > 0.0f ? (DeltaPixels / SliderPixelWidth) * MaximumTime : 0.0f;

        switch (ActiveDragMode)
        {
        case ERangeBarDragMode::StartHandle:
            EditorState->SetVisibleRange(DragVisibleStart + DeltaTime, DragVisibleEnd);
            break;
        case ERangeBarDragMode::EndHandle:
            EditorState->SetVisibleRange(DragVisibleStart, DragVisibleEnd + DeltaTime);
            break;
        case ERangeBarDragMode::Thumb:
            EditorState->SetVisibleRange(DragVisibleStart + DeltaTime, DragVisibleEnd + DeltaTime);
            break;
        case ERangeBarDragMode::None:
        default:
            break;
        }
    }

    if (ImGui::IsItemDeactivated())
    {
        ActiveDragMode = ERangeBarDragMode::None;
    }

    VisibleStart = EditorState->VisibleTimeStart;
    VisibleEnd = EditorState->VisibleTimeEnd;
    const float DrawThumbMinX = TimeToSliderX(VisibleStart);
    const float DrawThumbMaxX = TimeToSliderX(VisibleEnd);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    const float TrackMidY = (SliderMin.y + SliderMax.y) * 0.5f;
    DrawList->AddRectFilled(
        ImVec2(SliderMin.x, TrackMidY - 2.0f),
        ImVec2(SliderMax.x, TrackMidY + 2.0f),
        IM_COL32(72, 76, 84, 255),
        2.0f);
    DrawList->AddRectFilled(
        ImVec2(DrawThumbMinX, SliderMin.y + 1.0f),
        ImVec2(DrawThumbMaxX, SliderMax.y - 1.0f),
        IM_COL32(154, 156, 160, 255),
        3.0f);
    DrawList->AddRect(
        ImVec2(DrawThumbMinX, SliderMin.y + 1.0f),
        ImVec2(DrawThumbMaxX, SliderMax.y - 1.0f),
        IM_COL32(206, 208, 214, 255),
        3.0f);
    DrawList->AddRectFilled(
        ImVec2(DrawThumbMinX - 8.5f, SliderMin.y),
        ImVec2(DrawThumbMinX + 8.5f, SliderMax.y),
        IM_COL32(226, 228, 232, 255),
        1.0f);
    DrawList->AddRectFilled(
        ImVec2(DrawThumbMaxX - 8.5f, SliderMin.y),
        ImVec2(DrawThumbMaxX + 8.5f, SliderMax.y),
        IM_COL32(226, 228, 232, 255),
        1.0f);

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", "Adjust the visible timeline range");
    }

    ImGui::SetCursorPosX(SliderStartX + SliderWidth + GroupSpacing);
    DrawRangeLabel(
        "##AnimSequenceSequenceEndEdit",
        RightLabels[0],
        true,
        [&](float NewTime) { EditorState->SetTimelineRange(EditorState->GetTimelineRangeStart(), NewTime); },
        SequenceEnd);
    ImGui::SameLine(0.0f, LabelSpacing);
    DrawRangeLabel(
        "##AnimSequencePlaybackEndDisplay",
        RightLabels[1],
        true,
        [&](float NewTime)
        {
            if (PreviewController)
            {
                PreviewController->SetPlaybackRange(EditorState->GetPlaybackRangeStart(), NewTime);
                EditorState->SetPlaybackRange(
                    PreviewController->GetPlaybackRangeStart(),
                    PreviewController->GetPlaybackRangeEnd());
            }
        },
        PlaybackEnd);
    ImGui::SameLine(0.0f, LabelSpacing);
    DrawRangeLabel(
        "##AnimSequenceViewEndEdit",
        RightLabels[2],
        true,
        [&](float NewTime) { EditorState->SetVisibleRange(EditorState->VisibleTimeStart, NewTime); },
        EditorState->VisibleTimeEnd);
    ImGui::Dummy(ImVec2(EdgePadding, 0.0f));
}

void FAnimationSequenceEditorWidget::RenderPlaybackSpeedPopup()
{
    if (!EditorState)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(252.0f, 0.0f), ImGuiCond_Appearing);
    if (!ImGui::BeginPopup("##AnimSequencePlaybackSpeedPopup"))
    {
        return;
    }

    ImGui::Spacing();
    ImGui::TextDisabled("PLAYBACK SPEED");

    auto ApplyPlaybackRate = [&](float NewRate)
    {
        const float Direction = EditorState->PlayRate < 0.0f ? -1.0f : 1.0f;
        const float AppliedRate = std::max(std::fabs(NewRate), 0.0001f) * Direction;
        PlaybackSpeedCustomValue = std::fabs(NewRate);
        if (PreviewController)
        {
            PreviewController->SetPlayRate(AppliedRate);
            EditorState->PlayRate = PreviewController->GetPlayRate();
        }
        else
        {
            EditorState->PlayRate = AppliedRate;
        }
    };

    constexpr float PresetRates[] = { 0.1f, 0.25f, 0.5f, 0.75f, 1.0f, 2.0f, 5.0f, 10.0f };
    for (float PresetRate : PresetRates)
    {
        const FString Label = FormatPlaybackRateLabel(PresetRate);
        if (ImGui::Selectable(Label.c_str(), IsPlaybackRatePresetSelected(EditorState->PlayRate, PresetRate)))
        {
            ApplyPlaybackRate(PresetRate);
        }
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Custom Speed:");
    ImGui::SetNextItemWidth(-1.0f);
    const bool bChangedByEnter = ImGui::InputFloat(
        "##AnimSequenceCustomPlaybackSpeed",
        &PlaybackSpeedCustomValue,
        0.0f,
        0.0f,
        "%.2f",
        ImGuiInputTextFlags_EnterReturnsTrue);
    if ((bChangedByEnter || ImGui::IsItemDeactivatedAfterEdit()) && std::isfinite(PlaybackSpeedCustomValue))
    {
        PlaybackSpeedCustomValue = std::max(std::fabs(PlaybackSpeedCustomValue), 0.0001f);
        ApplyPlaybackRate(PlaybackSpeedCustomValue);
    }

    ImGui::Separator();
    ImGui::Checkbox("Snap to Frames", &EditorState->bSnapToFrames);

    ImGui::EndPopup();
}

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

void FAnimationSequenceEditorWidget::RenderTransportControls(bool bCanTimelineControl, bool bCanPlaybackControl, float StripHeight)
{
    (void)StripHeight;
    EnsurePlaybackControlIconsLoaded();
    const ImVec2 ButtonSize(PlaybackIconButtonSize, PlaybackIconButtonSize);
    const FTransportStripVerticalCenter VerticalCenter
    {
        0.0f,
        std::max(ImGui::GetWindowHeight(), ImGui::GetContentRegionAvail().y)
    };
    const float IconButtonItemHeight = ButtonSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;

    auto CenterButtonCursor = [&]()
    {
        VerticalCenter.AlignCursorY(IconButtonItemHeight);
    };

    const float PlaybackRateMagnitude = std::max(std::fabs(EditorState ? EditorState->PlayRate : 1.0f), 0.0001f);
    const bool bIsPlaying = PreviewController && PreviewController->IsPlaying();
    const bool bIsReversePlaying = bIsPlaying && EditorState && EditorState->PlayRate < 0.0f;
    const bool bIsForwardPlaying = bIsPlaying && EditorState && EditorState->PlayRate >= 0.0f;

    if (!bCanTimelineControl)
    {
        ImGui::BeginDisabled();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(PlaybackControlSpacing, PlaybackControlSpacing));
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.40f, 0.54f, 0.48f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.24f, 0.32f, 0.44f, 0.60f));

    CenterButtonCursor();
    if (DrawPlaybackControlButton(
            "##AnimSequenceJumpToStart",
            PlaybackControlIcons.JumpToStart,
            "|<",
            "Jump to Start",
            ButtonSize) &&
        PreviewController)
    {
        PreviewController->JumpToStart();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }
    ImGui::SameLine();
    CenterButtonCursor();
    if (DrawPlaybackControlButton(
            "##AnimSequenceStepPrevious",
            PlaybackControlIcons.StepPrevious,
            "<F",
            "Previous Frame",
            ButtonSize) &&
        PreviewController)
    {
        PreviewController->StepToPreviousFrame();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }
    ImGui::SameLine();

    if (!bCanPlaybackControl)
    {
        ImGui::BeginDisabled();
    }

    CenterButtonCursor();
    if (DrawPlaybackControlButton(
            "##AnimSequenceReverse",
            bIsReversePlaying ? PlaybackControlIcons.Pause : PlaybackControlIcons.PlayReverse,
            bIsReversePlaying ? "Pause" : "Rev",
            bIsReversePlaying ? "Pause Reverse Playback" : "Play Reverse",
            ButtonSize) &&
        PreviewController)
    {
        if (bIsReversePlaying)
        {
            PreviewController->Pause();
            EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
        }
        else
        {
            PreviewController->SetPlayRate(-PlaybackRateMagnitude);
            EditorState->PlayRate = PreviewController->GetPlayRate();
            PreviewController->Play();
        }
    }
    ImGui::SameLine();
    CenterButtonCursor();
    DrawPlaybackControlButton(
        "##AnimSequenceRecord",
        PlaybackControlIcons.Record,
        "Rec",
        "Record Placeholder",
        ButtonSize);
    ImGui::SameLine();
    CenterButtonCursor();
    if (DrawPlaybackControlButton(
            "##AnimSequencePlay",
            bIsForwardPlaying ? PlaybackControlIcons.Pause : PlaybackControlIcons.PlayForward,
            bIsForwardPlaying ? "Pause" : "Play",
            bIsForwardPlaying ? "Pause Playback" : "Play",
            ButtonSize) &&
        PreviewController)
    {
        if (bIsForwardPlaying)
        {
            PreviewController->Pause();
            EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
        }
        else
        {
            PreviewController->SetPlayRate(PlaybackRateMagnitude);
            EditorState->PlayRate = PreviewController->GetPlayRate();
            PreviewController->Play();
        }
    }

    if (!bCanPlaybackControl)
    {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    CenterButtonCursor();
    if (DrawPlaybackControlButton(
            "##AnimSequenceStepNext",
            PlaybackControlIcons.StepNext,
            "F>",
            "Next Frame",
            ButtonSize) &&
        PreviewController)
    {
        PreviewController->StepToNextFrame();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }
    ImGui::SameLine();
    CenterButtonCursor();
    if (DrawPlaybackControlButton(
            "##AnimSequenceJumpToEnd",
            PlaybackControlIcons.JumpToEnd,
            ">|",
            "Jump to End",
            ButtonSize) &&
        PreviewController)
    {
        PreviewController->JumpToEnd();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }
    ImGui::SameLine(0.0f, PlaybackControlSpacing);

    bool bLooping = EditorState->bLoop;
    CenterButtonCursor();
    if (DrawPlaybackControlButton(
            "##AnimSequenceLoopToggle",
            bLooping ? PlaybackControlIcons.LoopEnabled : PlaybackControlIcons.LoopDisabled,
            "Loop",
            bLooping ? "Loop Enabled" : "Loop Disabled",
            ButtonSize,
            bLooping) &&
        PreviewController)
    {
        const bool bNewLooping = !bLooping;
        PreviewController->SetLooping(bNewLooping);
        EditorState->bLoop = bNewLooping;
        bLooping = bNewLooping;
    }

    ImGui::SameLine(0.0f, PlaybackControlSpacing);
    CenterButtonCursor();
    const FString PlaybackRateButtonLabel = FormatPlaybackRateLabel(EditorState->PlayRate);
    const float PlaybackRateButtonWidth =
        std::max(
            50.0f,
            ImGui::CalcTextSize(PlaybackRateButtonLabel.c_str()).x +
                ImGui::GetStyle().FramePadding.x * 2.0f +
                8.0f);
    const ImVec2 PlaybackRateButtonSize(PlaybackRateButtonWidth, IconButtonItemHeight);
    const bool bPlaybackRatePressed =
        ImGui::InvisibleButton("##AnimSequencePlaybackRateButton", PlaybackRateButtonSize);
    const ImVec2 PlaybackRateButtonMin = ImGui::GetItemRectMin();
    const ImVec2 PlaybackRateButtonMax = ImGui::GetItemRectMax();
    const bool bPlaybackRateHovered = ImGui::IsItemHovered();
    const bool bPlaybackRateActive = ImGui::IsItemActive();
    ImDrawList* PlaybackRateDrawList = ImGui::GetWindowDrawList();
    if (bPlaybackRateHovered || bPlaybackRateActive)
    {
        PlaybackRateDrawList->AddRectFilled(
            PlaybackRateButtonMin,
            PlaybackRateButtonMax,
            ImGui::GetColorU32(bPlaybackRateActive ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered),
            ImGui::GetStyle().FrameRounding);
    }

    const ImVec2 PlaybackRateTextSize = ImGui::CalcTextSize(PlaybackRateButtonLabel.c_str());
    const ImVec2 PlaybackRateTextPos =
        VerticalCenter.ComputeCenteredTextPos(PlaybackRateButtonMin, PlaybackRateButtonMax, PlaybackRateTextSize);
    PlaybackRateDrawList->AddText(
        PlaybackRateTextPos,
        ImGui::GetColorU32(ImGuiCol_Text),
        PlaybackRateButtonLabel.c_str());

    if (bPlaybackRatePressed)
    {
        PlaybackSpeedCustomValue = std::fabs(EditorState->PlayRate);
        ImGui::OpenPopup("##AnimSequencePlaybackSpeedPopup");
    }
    if (bPlaybackRateHovered)
    {
        ImGui::SetTooltip("%s", "Playback Speed Options");
    }
    RenderPlaybackSpeedPopup();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

    if (!bCanTimelineControl)
    {
        ImGui::EndDisabled();
    }
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
    const bool bHasSelection = Document->GetSelectedNotify() != nullptr;
    RenderSelectedNotifyToolbar(bHasSelection, DefaultTrackIndex);
    ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing * 0.5f));

    const FAnimNotifyEvent* SelectedNotify = Document->GetSelectedNotify();
    if (!SelectedNotify)
    {
        NotifyDetailsBoundStableId.clear();
        NotifyNameEditBuffer.fill('\0');
        ResetNotifyPayloadFieldBuffers();
        NotifyPayloadEditBuffer.fill('\0');
        ImGui::TextDisabled("Select a notify marker to edit it.");
        return;
    }

    const FAnimNotifyEvent NotifySnapshot = *SelectedNotify;
    SyncNotifyDetailsBuffers(NotifySnapshot);
    const FAnimNotifyValidationReport SelectedValidationReport = Document->BuildSelectedNotifyValidationReport();
    RenderSelectedNotifyBasicSection(NotifySnapshot, SelectedValidationReport);

    const FAnimNotifyEvent* CurrentNotify = Document->GetSelectedNotify();
    const FString CurrentNotifyClassName =
        CurrentNotify ? CurrentNotify->GetResolvedNotifyClassName() : NotifySnapshot.GetResolvedNotifyClassName();

    RenderSelectedNotifyTimingSection(NotifySnapshot, SelectedValidationReport);
    RenderSelectedNotifyPayloadSection(CurrentNotifyClassName, SelectedValidationReport);
    RenderSelectedNotifyVisualSection(NotifySnapshot);
    RenderSelectedNotifyInlineValidationSection(SelectedValidationReport);
    RenderSelectedNotifyMetaFooter(NotifySnapshot);
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
    DrawNotifyPanelSectionHeader("Basic");

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

    const FAnimNotifyEvent* CurrentNotify = Document->GetSelectedNotify();
    const EAnimNotifyEventType CurrentNotifyType =
        CurrentNotify ? CurrentNotify->EventType : NotifySnapshot.EventType;
    const FString CurrentNotifyClassName =
        CurrentNotify ? CurrentNotify->GetResolvedNotifyClassName() : NotifySnapshot.GetResolvedNotifyClassName();
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
    DrawNotifyPanelSectionHeader("Timing");

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
    DrawNotifyPanelSectionHeader("Payload");

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
    DrawNotifyPanelSectionHeader("Visual");

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
    DrawNotifyPanelSectionHeader("Validation");
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
    ImGui::TextDisabled("%d errors, %d warnings, %d info", Report.ErrorCount, Report.WarningCount, Report.InfoCount);
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
        ImGui::TextDisabled(
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

void FAnimationSequenceEditorWidget::RenderCurveInspectionPanel()
{
    ImGui::Spacing();
    ImGui::TextDisabled("Selected Curve");

    const UAnimDataModel* DataModel = AnimationSequenceViewer::GetValidAnimDataModel(Sequence);
    if (!DataModel || DataModel->CurveData.FloatCurves.empty())
    {
        ImGui::TextDisabled("No float curves in this sequence.");
        return;
    }

    const FFloatCurve* SelectedCurve = GetSelectedCurve(Sequence, EditorState);
    if (!SelectedCurve)
    {
        const TArray<FAnimationSequenceCurveViewGroup> CurveGroups =
            EditorState ? AnimationSequenceCurveFilter::BuildCurveViewGroups(Sequence, *EditorState) : TArray<FAnimationSequenceCurveViewGroup>();
        const int32 VisibleCurveCount = GetVisibleCurveCount(CurveGroups);
        ImGui::TextDisabled(
            "%d visible curves / %d total. Select a curve from the outliner or graph row.",
            VisibleCurveCount,
            static_cast<int32>(DataModel->CurveData.FloatCurves.size()));
        for (const FAnimationSequenceCurveViewGroup& Group : CurveGroups)
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

    const FCurveInspectionSnapshot Snapshot = BuildCurveInspectionSnapshot(*SelectedCurve, *EditorState);

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
        AnimationSequenceCurveFilter::SetExclusiveCurveTypeVisible(*EditorState, SelectedCurve->CurveType);
    }

    ImGui::SameLine();
    if (ImGui::Button("Show All"))
    {
        AnimationSequenceCurveFilter::SetAllCurveTypesEnabled(*EditorState, true);
    }

    ImGui::BulletText("%s", SelectedCurve->CurveName.ToString().c_str());
    ImGui::BulletText("Type %s", AnimationCurveTypeToString(SelectedCurve->CurveType).c_str());
    ImGui::BulletText("Source %s", AnimationCurveSourceKindToString(SelectedCurve->SourceKind).c_str());
    ImGui::BulletText("%d keys", static_cast<int32>(SelectedCurve->Keys.size()));
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
            EditorState->HoveredCurveNearestKeyIndex < static_cast<int32>(SelectedCurve->Keys.size()))
        {
            const FCurveKey& HoveredKey = SelectedCurve->Keys[EditorState->HoveredCurveNearestKeyIndex];
            ImGui::BulletText(
                "Hovered Nearest Key #%d  %.3fs  %.3f",
                EditorState->HoveredCurveNearestKeyIndex,
                HoveredKey.Time,
                HoveredKey.Value);
        }
    }
}

void FAnimationSequenceEditorWidget::RenderRecentNotifySummary() const
{
    RenderRecentFiredNotifyTab();
}

void FAnimationSequenceEditorWidget::RenderRecentFiredNotifyTab() const
{
    if (!PreviewController)
    {
        ImGui::TextDisabled("Preview controller is unavailable.");
        return;
    }

    const TArray<FAnimNotifyEvent>& RecentNotifies = PreviewController->GetRecentFiredNotifyEvents();
    if (RecentNotifies.empty())
    {
        if (ImGui::BeginChild("##RecentFiredNotifyTabEmpty", ImVec2(0.0f, 0.0f), true))
        {
            ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing));
            ImGui::TextDisabled("No notify fired during the latest preview update.");
            ImGui::TextDisabled("Preview playback activity will appear here.");
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
        ImGui::Dummy(ImVec2(0.0f, NotifyPanelSectionSpacing));
        ImGui::TextDisabled("Notify event log is reserved for future debug output.");
        ImGui::TextDisabled("Use Validation and Recent Fired for the current notify debug flow.");
    }
    ImGui::EndChild();
}
