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
#include "Core/Paths.h"
#include "Engine/Asset/CurveFloatAsset.h"
#include "Object/ObjectFactory.h"
#include "Engine/Runtime/WindowsWindow.h"
#include "Render/Common/RenderTypes.h"

#include "ImGui/imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
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
        TArray<const FTypeInfo*> RegisteredTypes;
        FObjectFactory::Get().GetRegisteredTypeInfos(RegisteredTypes);

        const FTypeInfo* RequiredBaseType =
            EventType == EAnimNotifyEventType::NotifyState ? &UAnimNotifyState::s_TypeInfo : &UAnimNotify::s_TypeInfo;
        const FTypeInfo* ExcludedBaseType =
            EventType == EAnimNotifyEventType::NotifyState ? nullptr : &UAnimNotifyState::s_TypeInfo;

        TArray<FString> Options;
        for (const FTypeInfo* TypeInfo : RegisteredTypes)
        {
            if (!TypeInfo || !TypeInfo->name || !TypeInfo->IsA(RequiredBaseType))
            {
                continue;
            }

            if (ExcludedBaseType && TypeInfo->IsA(ExcludedBaseType))
            {
                continue;
            }

            Options.push_back(TypeInfo->name);
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

    float GetCurveSectionHeight(
        const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups,
        const FAnimationSequenceEditorState& State)
    {
        float Height = FAnimationSequenceSequencerLayout::SectionHeaderHeight;
        if (!State.bCurvesExpanded)
        {
            return Height;
        }

        Height += FAnimationSequenceSequencerLayout::TrackAreaPadding;
        for (int32 GroupIndex = 0; GroupIndex < static_cast<int32>(CurveGroups.size()); ++GroupIndex)
        {
            const FAnimationSequenceCurveViewGroup& Group = CurveGroups[GroupIndex];
            Height += FAnimationSequenceSequencerLayout::CurveGroupHeaderHeight;

            if (Group.bVisible && !Group.VisibleEntries.empty())
            {
                Height += FAnimationSequenceSequencerLayout::CurveGroupHeaderSpacing;
                Height +=
                    FAnimationSequenceSequencerLayout::CurveTrackRowHeight * static_cast<float>(Group.VisibleEntries.size()) +
                    FAnimationSequenceSequencerLayout::CurveTrackRowSpacing * static_cast<float>(std::max(0, static_cast<int32>(Group.VisibleEntries.size()) - 1));
            }

            if (GroupIndex + 1 < static_cast<int32>(CurveGroups.size()))
            {
                Height += FAnimationSequenceSequencerLayout::CurveGroupHeaderSpacing;
            }
        }

        Height += FAnimationSequenceSequencerLayout::TrackAreaPadding;
        return Height;
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
}

void FAnimationSequenceEditorWidget::ResetNotifyPayloadFieldBuffers()
{
    NotifySoundCueEditBuffer.fill('\0');
    NotifySocketNameEditBuffer.fill('\0');
    NotifyComponentNameEditBuffer.fill('\0');
    NotifyAttackIdEditBuffer.fill('\0');
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

    CopyPayloadFieldToBuffer(Payload, SoundCueField, NotifySoundCueEditBuffer);
    CopyPayloadFieldToBuffer(Payload, SocketNameField, NotifySocketNameEditBuffer);
    CopyPayloadFieldToBuffer(Payload, ComponentNameField, NotifyComponentNameEditBuffer);
    CopyPayloadFieldToBuffer(Payload, AttackIdField, NotifyAttackIdEditBuffer);
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
    const float FooterHeight = ImGui::GetTextLineHeightWithSpacing() * 2.0f + 16.0f;
    const float AvailableHeight = std::max(ImGui::GetContentRegionAvail().y - FooterHeight, 300.0f);
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

    ImGui::Spacing();
    RenderFooterStatus();
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
    const TArray<FAnimationSequenceCurveViewGroup> CurveGroups =
        EditorState ? AnimationSequenceCurveFilter::BuildCurveViewGroups(Sequence, *EditorState) : TArray<FAnimationSequenceCurveViewGroup>();

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
    RenderTransportBar();

    const float DetailsPaneHeight =
        std::min(FAnimationSequenceSequencerLayout::DetailsPanelHeight, SequencerHeight * 0.34f);
    const float SplitPaneHeight = std::max(
        SequencerHeight -
        FAnimationSequenceSequencerLayout::TransportBarHeight -
        DetailsPaneHeight -
        SequencerHeaderPadding,
        140.0f);

    const float MaxOutlinerWidth = std::max(
        FAnimationSequenceSequencerLayout::OutlinerMinWidth,
        ImGui::GetContentRegionAvail().x - 280.0f);
    RenderSequencerSplitPane(SplitPaneHeight, DetailsPaneHeight, MaxOutlinerWidth, CurveGroups);
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderTransportBar()
{
    ImGui::BeginChild(
        "##AnimationSequenceTransportBar",
        ImVec2(0.0f, FAnimationSequenceSequencerLayout::TransportBarHeight),
        true);
    ImGui::TextDisabled("Sequencer");

    const bool bCanTimelineControl =
        PreviewController != nullptr && EditorState != nullptr && EditorState->HasTimelineData();
    const bool bCanPlaybackControl = PreviewController != nullptr && PreviewController->HasValidPreview();

    RenderTransportControls(bCanTimelineControl, bCanPlaybackControl);
    RenderPlaybackSummary();
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderSequencerSplitPane(
    float SplitPaneHeight,
    float DetailsPaneHeight,
    float MaxOutlinerWidth,
    const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups)
{
    EditorState->TrackOutlinerWidth = std::clamp(
        EditorState->TrackOutlinerWidth,
        FAnimationSequenceSequencerLayout::OutlinerMinWidth,
        MaxOutlinerWidth);

    const int32 NotifyTrackCount = Document ? Document->GetNotifyTrackCount() : 0;
    const float TrackBodyHeight =
        FAnimationSequenceSequencerLayout::GetNotifySectionHeight(NotifyTrackCount, EditorState->bNotifiesExpanded) +
        FAnimationSequenceSequencerLayout::SectionGap +
        GetCurveSectionHeight(CurveGroups, *EditorState);
    const float TrackViewportHeight =
        SplitPaneHeight -
        FAnimationSequenceSequencerLayout::RulerHeight -
        FAnimationSequenceTimelineGeometry::VerticalPadding * 2.0f;
    const float MaxScrollY = std::max(
        0.0f,
        TrackBodyHeight + FAnimationSequenceSequencerLayout::TrackAreaPadding * 2.0f - TrackViewportHeight);
    EditorState->SequencerScrollY = std::clamp(EditorState->SequencerScrollY, 0.0f, MaxScrollY);

    ImGui::BeginChild("##AnimationSequenceSequencerSplit", ImVec2(0.0f, SplitPaneHeight), false, ImGuiWindowFlags_NoScrollbar);
    RenderTrackOutlinerPane(EditorState->TrackOutlinerWidth, SplitPaneHeight, CurveGroups);

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::InvisibleButton(
        "##AnimationSequenceOutlinerSplitter",
        ImVec2(FAnimationSequenceSequencerLayout::OutlinerSplitterWidth, SplitPaneHeight));
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
        std::max(ImGui::GetContentRegionAvail().x, 1.0f),
        SplitPaneHeight,
        CurveGroups);

    ImGui::EndChild();
    RenderSelectionDetailsPane(DetailsPaneHeight);
}

void FAnimationSequenceEditorWidget::RenderTrackOutlinerPane(
    float Width,
    float Height,
    const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups)
{
    ImGui::BeginChild(
        "##AnimationSequenceTrackOutlinerPane",
        ImVec2(Width, Height),
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    TrackOutlinerWidget.Render(*EditorState, Document, CurveGroups);
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
        std::fabs(ImGui::GetIO().MouseWheel) > 0.0f)
    {
        EditorState->SequencerScrollY = std::max(EditorState->SequencerScrollY - ImGui::GetIO().MouseWheel * 24.0f, 0.0f);
    }
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderTimelineCanvasPane(
    float Width,
    float Height,
    const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups)
{
    ImGui::BeginChild(
        "##AnimationSequenceTimelineCanvasPane",
        ImVec2(Width, Height),
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const int32 NotifyTrackCount = Document ? Document->GetNotifyTrackCount() : 0;
    const float RequiredCanvasHeight =
        FAnimationSequenceTimelineGeometry::VerticalPadding * 2.0f +
        TimelineWidget.GetRulerHeight() +
        FAnimationSequenceSequencerLayout::TrackAreaPadding * 2.0f +
        FAnimationSequenceSequencerLayout::GetNotifySectionHeight(NotifyTrackCount, EditorState->bNotifiesExpanded) +
        FAnimationSequenceSequencerLayout::SectionGap +
        GetCurveSectionHeight(CurveGroups, *EditorState);

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

    TimelineWidget.RenderCanvas(*EditorState, PreviewController, Geometry, bCanvasHovered, bCanvasActive);

    const float NotifySectionTop =
        Geometry.TrackTop +
        FAnimationSequenceSequencerLayout::TrackAreaPadding -
        EditorState->SequencerScrollY;
    NotifyLaneWidget.RenderRows(*EditorState, Document, Geometry, NotifySectionTop);

    const float CurveSectionTop =
        NotifySectionTop +
        FAnimationSequenceSequencerLayout::GetNotifySectionHeight(NotifyTrackCount, EditorState->bNotifiesExpanded) +
        FAnimationSequenceSequencerLayout::SectionGap;
    CurveTrackWidget.RenderRows(*EditorState, Geometry, CurveSectionTop, CurveGroups);
    if (Document &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        EditorState->HoveredCurveIndex >= 0)
    {
        Document->ClearNotifySelection();
    }

    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderSelectionDetailsPane(float Height)
{
    ImGui::BeginChild("##AnimationSequenceSelectionDetails", ImVec2(0.0f, Height), true);
    ImGui::TextDisabled("Selection & Activity");
    if (ImGui::BeginTable("##AnimationSequenceSelectionDetailsTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Selection", ImGuiTableColumnFlags_WidthStretch, 0.55f);
        ImGui::TableSetupColumn("Activity", ImGuiTableColumnFlags_WidthStretch, 0.45f);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        RenderNotifyDetailsPanel();
        RenderCurveInspectionPanel();

        ImGui::TableSetColumnIndex(1);
        RenderRecentNotifySummary();
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

void FAnimationSequenceEditorWidget::RenderTransportControls(bool bCanTimelineControl, bool bCanPlaybackControl)
{
    if (!bCanTimelineControl)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("|<") && PreviewController)
    {
        PreviewController->JumpToStart();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }
    ImGui::SameLine();
    if (ImGui::Button("< Frame") && PreviewController)
    {
        PreviewController->StepToPreviousFrame();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }
    ImGui::SameLine();

    if (!bCanPlaybackControl)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Play") && PreviewController)
    {
        PreviewController->Play();
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause") && PreviewController)
    {
        PreviewController->Pause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop") && PreviewController)
    {
        PreviewController->Stop();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }

    if (!bCanPlaybackControl)
    {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Frame >") && PreviewController)
    {
        PreviewController->StepToNextFrame();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }
    ImGui::SameLine();
    if (ImGui::Button(">|") && PreviewController)
    {
        PreviewController->JumpToEnd();
        EditorState->SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }
    ImGui::SameLine(0.0f, 18.0f);

    bool bLooping = EditorState->bLoop;
    if (ImGui::Checkbox("Loop", &bLooping) && PreviewController)
    {
        PreviewController->SetLooping(bLooping);
        EditorState->bLoop = bLooping;
    }

    ImGui::SameLine();
    float PlaybackRate = EditorState->PlayRate;
    ImGui::SetNextItemWidth(130.0f);
    if (ImGui::DragFloat("Play Rate", &PlaybackRate, 0.05f, -4.0f, 4.0f, "%.2fx") && PreviewController)
    {
        PreviewController->SetPlayRate(PlaybackRate);
        EditorState->PlayRate = PreviewController->GetPlayRate();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Snap", &EditorState->bSnapToFrames);

    if (!bCanTimelineControl)
    {
        ImGui::EndDisabled();
    }
}

void FAnimationSequenceEditorWidget::RenderPlaybackSummary() const
{
    const int32 CurrentFrame = EditorState->TimeToFrameIndex(EditorState->CurrentTime);
    const int32 TotalFrames = std::max(EditorState->TotalFrames, 0);
    ImGui::Spacing();
    ImGui::Text(
        "Time %.3fs / %.3fs    Frame %d / %d    Rate %.2f fps    Playback %s",
        EditorState->CurrentTime,
        EditorState->SequenceLength,
        TotalFrames > 0 ? CurrentFrame + 1 : 0,
        TotalFrames,
        EditorState->GetDisplayFPS(),
        PreviewController && PreviewController->IsPlaying() ? "Playing" : "Paused");
}

void FAnimationSequenceEditorWidget::RenderFooterStatus() const
{
    ImGui::TextDisabled(
        "%s",
        PreviewController ? PreviewController->GetTimelineStatusText().c_str() : "Timeline is unavailable.");

    if (!(PreviewController && PreviewController->HasSequence()))
    {
        ImGui::TextDisabled("Animation sequence is unavailable.");
    }

    if (Document)
    {
        const FAnimNotifyValidationReport DocumentReport = Document->BuildDocumentNotifyValidationReport();
        const ImVec4 SummaryColor = GetValidationSeverityColor(
            DocumentReport.ErrorCount > 0
                ? EAnimNotifyValidationSeverity::Error
                : (DocumentReport.WarningCount > 0
                    ? EAnimNotifyValidationSeverity::Warning
                    : EAnimNotifyValidationSeverity::Info));
        ImGui::TextColored(
            SummaryColor,
            "Notify Validation: %s",
            FormatAnimNotifyValidationSummary(DocumentReport).c_str());

        if (!Document->GetLastNotifyValidationStatusText().empty())
        {
            ImGui::TextDisabled("Last Save: %s", Document->GetLastNotifyValidationStatusText().c_str());
        }
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
            if (ImGui::DragFloat(Field.Label.c_str(), &FieldValue, 0.01f, 0.0f, 100.0f, "%.3f"))
            {
                if (Document->SetSelectedNotifyPayloadFloatValue(Field.Key, FieldValue))
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
            ImGui::TextDisabled("%s", Field.HelpText.c_str());
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
    ImGui::TextDisabled("Selected Notify");

    if (!Document || !EditorState)
    {
        ImGui::TextDisabled("Notify editing is unavailable.");
        return;
    }

    const int32 DefaultTrackIndex = std::max(EditorState->SelectedNotifyTrackIndex, 0);
    if (ImGui::Button("Add Notify"))
    {
        Document->AddNotifyAtTime(DefaultTrackIndex, EditorState->CurrentTime);
    }

    const bool bHasSelection = Document->GetSelectedNotify() != nullptr;
    ImGui::SameLine();
    if (!bHasSelection)
    {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Delete Selected"))
    {
        Document->DeleteSelectedNotify();
    }
    if (!bHasSelection)
    {
        ImGui::EndDisabled();
    }

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
    RenderNotifyValidationSummary(SelectedValidationReport);
    ImGui::Checkbox("Show Validation Details", &EditorState->bShowNotifyValidationDetails);
    RenderNotifyValidationIssues(SelectedValidationReport, EAnimNotifyValidationField::General);

    if (ImGui::InputText("Name", NotifyNameEditBuffer.data(), NotifyNameEditBuffer.size()))
    {
        Document->SetSelectedNotifyName(FName(NotifyNameEditBuffer.data()));
    }
    RenderNotifyValidationIssues(SelectedValidationReport, EAnimNotifyValidationField::Name);

    const char* NotifyTypeLabels[] = { "Notify", "Notify State" };
    int32 NotifyTypeIndex = NotifySnapshot.EventType == EAnimNotifyEventType::NotifyState ? 1 : 0;
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
        ImGui::TextDisabled(
            "Test classes: %s / %s",
            "UAnimNotifyLog",
            "UAnimNotifyStateLog");
    }

    if (HasAnimNotifyPayloadSchema(CurrentNotifyClassName))
    {
        RenderStructuredNotifyPayloadEditor(CurrentNotifyClassName, SelectedValidationReport);
    }
    else
    {
        RenderRawNotifyPayloadEditor(CurrentNotifyClassName, SelectedValidationReport, true);
    }

    float NotifyTime = NotifySnapshot.Time;
    const float MaxTime = std::max(EditorState->SequenceLength, 0.0f);
    if (ImGui::DragFloat("Time", &NotifyTime, 0.001f, 0.0f, MaxTime, "%.3f s"))
    {
        Document->SetSelectedNotifyTime(NotifyTime, EditorState->bSnapToFrames);
    }

    float NotifyDuration = NotifySnapshot.Duration;
    if (ImGui::DragFloat("Duration", &NotifyDuration, 0.001f, 0.0f, MaxTime, "%.3f s"))
    {
        Document->SetSelectedNotifyDuration(NotifyDuration);
    }
    RenderNotifyValidationIssues(SelectedValidationReport, EAnimNotifyValidationField::Duration);

    float Color[4] =
    {
        NotifySnapshot.Color.R,
        NotifySnapshot.Color.G,
        NotifySnapshot.Color.B,
        NotifySnapshot.Color.A
    };
    if (ImGui::ColorEdit4("Color", Color))
    {
        Document->SetSelectedNotifyColor(FColor(
            std::clamp(Color[0], 0.0f, 1.0f),
            std::clamp(Color[1], 0.0f, 1.0f),
            std::clamp(Color[2], 0.0f, 1.0f),
            std::clamp(Color[3], 0.0f, 1.0f)));
    }

    ImGui::TextDisabled(
        "Track %d  StableId %s",
        EditorState->SelectedNotifyTrackIndex + 1,
        NotifySnapshot.StableId.ToString().c_str());
}

void FAnimationSequenceEditorWidget::RenderCurveInspectionPanel() const
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

    float MinTime = 0.0f;
    float MaxTime = 0.0f;
    if (!SelectedCurve->Keys.empty())
    {
        MinTime = SelectedCurve->Keys.front().Time;
        MaxTime = SelectedCurve->Keys.front().Time;
        for (const FCurveKey& Key : SelectedCurve->Keys)
        {
            MinTime = std::min(MinTime, Key.Time);
            MaxTime = std::max(MaxTime, Key.Time);
        }
    }

    ImGui::BulletText("%s", SelectedCurve->CurveName.ToString().c_str());
    ImGui::BulletText("Type %s", AnimationCurveTypeToString(SelectedCurve->CurveType).c_str());
    ImGui::BulletText("Source %s", AnimationCurveSourceKindToString(SelectedCurve->SourceKind).c_str());
    ImGui::BulletText("%d keys", static_cast<int32>(SelectedCurve->Keys.size()));
    ImGui::BulletText("Range %.3fs - %.3fs", MinTime, MaxTime);
}

void FAnimationSequenceEditorWidget::RenderRecentNotifySummary() const
{
    ImGui::TextDisabled("Recent Fired Notifies");

    if (!PreviewController)
    {
        ImGui::TextDisabled("Preview controller is unavailable.");
        return;
    }

    const TArray<FAnimNotifyEvent>& RecentNotifies = PreviewController->GetRecentFiredNotifyEvents();
    if (RecentNotifies.empty())
    {
        ImGui::TextDisabled("No notify fired during the latest preview update.");
        return;
    }

    constexpr int32 MaxVisibleEntries = 6;
    const int32 VisibleCount = std::min(static_cast<int32>(RecentNotifies.size()), MaxVisibleEntries);
    for (int32 Offset = 0; Offset < VisibleCount; ++Offset)
    {
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
