#include "Editor/Animation/AnimationSequenceTrackOutlinerWidget.h"

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Editor/Animation/AnimationSequenceEditorDocument.h"
#include "Editor/Animation/AnimationSequenceEditorState.h"
#include "Editor/Animation/AnimationSequenceSequencerLayout.h"
#include "Editor/Animation/AnimationSequenceTimelineGeometry.h"
#include "Editor/Animation/AnimationSequenceViewerUtils.h"
#include "Engine/Asset/CurveFloatAsset.h"

#include "ImGui/imgui.h"

#include <algorithm>
#include <string>

namespace
{
    ImU32 ToPanelColor(float Intensity, float Alpha = 1.0f)
    {
        const int32 Value = static_cast<int32>(std::clamp(Intensity, 0.0f, 1.0f) * 255.0f);
        const int32 AlphaValue = static_cast<int32>(std::clamp(Alpha, 0.0f, 1.0f) * 255.0f);
        return IM_COL32(Value, Value, Value, AlphaValue);
    }

    bool DrawSectionRow(const char* Id, const FString& Label, bool& bExpanded, const ImVec2& Min, const ImVec2& Max)
    {
        ImGui::SetCursorScreenPos(Min);
        ImGui::PushID(Id);
        ImGui::InvisibleButton("##SectionRow", ImVec2(Max.x - Min.x, Max.y - Min.y));
        const bool bHovered = ImGui::IsItemHovered();
        const bool bClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        if (bClicked)
        {
            bExpanded = !bExpanded;
        }

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRectFilled(Min, Max, bHovered ? ToPanelColor(0.20f) : ToPanelColor(0.15f), 4.0f);
        DrawList->AddRect(Min, Max, ToPanelColor(0.30f), 4.0f);

        const float ArrowCenterY = 0.5f * (Min.y + Max.y);
        const float ArrowX = Min.x + 10.0f;
        if (bExpanded)
        {
            DrawList->AddTriangleFilled(
                ImVec2(ArrowX - 4.0f, ArrowCenterY - 2.0f),
                ImVec2(ArrowX + 4.0f, ArrowCenterY - 2.0f),
                ImVec2(ArrowX, ArrowCenterY + 3.0f),
                IM_COL32(220, 225, 232, 255));
        }
        else
        {
            DrawList->AddTriangleFilled(
                ImVec2(ArrowX - 2.0f, ArrowCenterY - 4.0f),
                ImVec2(ArrowX - 2.0f, ArrowCenterY + 4.0f),
                ImVec2(ArrowX + 3.0f, ArrowCenterY),
                IM_COL32(220, 225, 232, 255));
        }

        DrawList->AddText(ImVec2(Min.x + 20.0f, Min.y + 4.0f), IM_COL32(230, 234, 242, 255), Label.c_str());
        ImGui::PopID();
        return bClicked;
    }

    bool DrawSelectableRow(
        int32 UniqueId,
        const FString& Label,
        const FString& RightLabel,
        bool bSelected,
        const ImVec2& Min,
        const ImVec2& Max)
    {
        ImGui::SetCursorScreenPos(Min);
        ImGui::PushID(UniqueId);
        ImGui::InvisibleButton("##Row", ImVec2(Max.x - Min.x, Max.y - Min.y));
        const bool bHovered = ImGui::IsItemHovered();
        const bool bClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        const ImU32 FillColor = bSelected
            ? IM_COL32(56, 87, 121, 220)
            : (bHovered ? IM_COL32(38, 43, 52, 220) : IM_COL32(28, 31, 38, 180));
        DrawList->AddRectFilled(Min, Max, FillColor, 4.0f);
        DrawList->AddRect(Min, Max, IM_COL32(255, 255, 255, 18), 4.0f);
        DrawList->AddText(ImVec2(Min.x + 16.0f, Min.y + 4.0f), IM_COL32(228, 232, 240, 255), Label.c_str());

        if (!RightLabel.empty())
        {
            const ImVec2 LabelSize = ImGui::CalcTextSize(RightLabel.c_str());
            DrawList->AddText(
                ImVec2(Max.x - LabelSize.x - 10.0f, Min.y + 4.0f),
                IM_COL32(153, 160, 171, 255),
                RightLabel.c_str());
        }

        ImGui::PopID();
        return bClicked;
    }
}

void FAnimationSequenceTrackOutlinerWidget::Render(
    FAnimationSequenceEditorState& State,
    FAnimationSequenceEditorDocument* Document,
    const UAnimSequence* Sequence)
{
    const ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
    const ImVec2 CanvasSize(
        std::max(1.0f, ImGui::GetContentRegionAvail().x),
        std::max(1.0f, ImGui::GetContentRegionAvail().y));
    ImGui::InvisibleButton("##AnimationSequenceTrackOutlinerCanvas", CanvasSize);

    const ImVec2 CanvasEnd(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y);
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    DrawList->AddRectFilled(CanvasPos, CanvasEnd, IM_COL32(22, 24, 29, 255));
    DrawList->AddRect(CanvasPos, CanvasEnd, IM_COL32(255, 255, 255, 20));

    const float HeaderTop = CanvasPos.y + FAnimationSequenceTimelineGeometry::VerticalPadding;
    const float HeaderBottom = HeaderTop + FAnimationSequenceSequencerLayout::RulerHeight;
    DrawList->AddRectFilled(
        ImVec2(CanvasPos.x + 1.0f, HeaderTop),
        ImVec2(CanvasEnd.x - 1.0f, HeaderBottom),
        IM_COL32(30, 34, 40, 255),
        4.0f);
    DrawList->AddText(ImVec2(CanvasPos.x + 10.0f, HeaderTop + 4.0f), IM_COL32(220, 224, 232, 255), "Tracks");

    const UAnimDataModel* DataModel = AnimationSequenceViewer::GetValidAnimDataModel(Sequence);
    const int32 CurveCount = DataModel ? static_cast<int32>(DataModel->CurveData.FloatCurves.size()) : 0;
    const int32 NotifyTrackCount = Document ? Document->GetNotifyTrackCount() : 0;

    const float TrackAreaTop =
        HeaderBottom +
        FAnimationSequenceSequencerLayout::TrackAreaPadding -
        State.SequencerScrollY;
    const float NotifySectionHeight =
        FAnimationSequenceSequencerLayout::GetNotifySectionHeight(NotifyTrackCount, State.bNotifiesExpanded);
    const float CurveSectionTop = TrackAreaTop + NotifySectionHeight + FAnimationSequenceSequencerLayout::SectionGap;

    DrawSectionRow(
        "NotifySection",
        "Notifies",
        State.bNotifiesExpanded,
        ImVec2(CanvasPos.x + 4.0f, TrackAreaTop),
        ImVec2(CanvasEnd.x - 4.0f, TrackAreaTop + FAnimationSequenceSequencerLayout::SectionHeaderHeight));

    if (State.bNotifiesExpanded)
    {
        const float RowsTop = TrackAreaTop + FAnimationSequenceSequencerLayout::SectionHeaderHeight + 4.0f;
        for (int32 TrackIndex = 0; TrackIndex < NotifyTrackCount; ++TrackIndex)
        {
            const float RowTop =
                RowsTop +
                TrackIndex *
                (FAnimationSequenceSequencerLayout::NotifyTrackRowHeight + FAnimationSequenceSequencerLayout::NotifyTrackRowSpacing);
            const float RowBottom = RowTop + FAnimationSequenceSequencerLayout::NotifyTrackRowHeight;

            const FAnimNotifyTrack* Track = Document ? Document->GetNotifyTrack(TrackIndex) : nullptr;
            const FString TrackLabel = Track && Track->TrackName.IsValid()
                ? Track->TrackName.ToString()
                : ("Track " + std::to_string(TrackIndex + 1));
            const FString EventCountLabel = Track ? std::to_string(static_cast<int32>(Track->Events.size())) : FString("0");
            const bool bSelected = State.SelectedNotifyTrackIndex == TrackIndex;

            if (DrawSelectableRow(
                1000 + TrackIndex,
                TrackLabel,
                EventCountLabel,
                bSelected,
                ImVec2(CanvasPos.x + 8.0f, RowTop),
                ImVec2(CanvasEnd.x - 8.0f, RowBottom)))
            {
                State.SelectedCurveIndex = -1;
                State.HoveredCurveIndex = -1;
                if (Track && !Track->Events.empty())
                {
                    const int32 EventIndex = std::clamp(State.SelectedNotifyEventIndex, 0, static_cast<int32>(Track->Events.size()) - 1);
                    Document->SelectNotify(TrackIndex, EventIndex);
                }
            }
        }
    }

    DrawSectionRow(
        "CurveSection",
        "Curves",
        State.bCurvesExpanded,
        ImVec2(CanvasPos.x + 4.0f, CurveSectionTop),
        ImVec2(CanvasEnd.x - 4.0f, CurveSectionTop + FAnimationSequenceSequencerLayout::SectionHeaderHeight));

    if (State.bCurvesExpanded && DataModel)
    {
        const float RowsTop = CurveSectionTop + FAnimationSequenceSequencerLayout::SectionHeaderHeight + 4.0f;
        for (int32 CurveIndex = 0; CurveIndex < CurveCount; ++CurveIndex)
        {
            const FFloatCurve& Curve = DataModel->CurveData.FloatCurves[CurveIndex];
            const float RowTop =
                RowsTop +
                CurveIndex *
                (FAnimationSequenceSequencerLayout::CurveTrackRowHeight + FAnimationSequenceSequencerLayout::CurveTrackRowSpacing);
            const float RowBottom = RowTop + FAnimationSequenceSequencerLayout::CurveTrackRowHeight;
            const bool bSelected = State.SelectedCurveIndex == CurveIndex;

            if (DrawSelectableRow(
                2000 + CurveIndex,
                Curve.CurveName.ToString(),
                std::to_string(static_cast<int32>(Curve.Keys.size())) + " keys",
                bSelected,
                ImVec2(CanvasPos.x + 8.0f, RowTop),
                ImVec2(CanvasEnd.x - 8.0f, RowBottom)))
            {
                State.SelectedCurveIndex = CurveIndex;
                State.HoveredCurveIndex = CurveIndex;
                if (Document)
                {
                    Document->ClearNotifySelection();
                }
            }
        }
    }
}
