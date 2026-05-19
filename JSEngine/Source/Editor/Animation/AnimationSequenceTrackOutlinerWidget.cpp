#include "Editor/Animation/AnimationSequenceTrackOutlinerWidget.h"

#include "Editor/Animation/AnimationSequenceEditorDocument.h"
#include "Editor/Animation/AnimationSequenceEditorState.h"
#include "Editor/Animation/AnimationSequenceSequencerLayout.h"
#include "Editor/Animation/AnimationSequenceTimelineGeometry.h"
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

    bool DrawCurveGroupRow(
        int32 UniqueId,
        const FAnimationSequenceCurveViewGroup& Group,
        const ImVec2& Min,
        const ImVec2& Max)
    {
        ImGui::SetCursorScreenPos(Min);
        ImGui::PushID(UniqueId);
        ImGui::InvisibleButton("##CurveGroupRow", ImVec2(Max.x - Min.x, Max.y - Min.y));
        const bool bHovered = ImGui::IsItemHovered();
        const bool bClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRectFilled(
            Min,
            Max,
            bHovered ? IM_COL32(33, 37, 45, 245) : IM_COL32(25, 28, 34, 230),
            4.0f);
        DrawList->AddRect(Min, Max, IM_COL32(255, 255, 255, 16), 4.0f);

        const ImVec2 CheckboxMin(Min.x + 10.0f, Min.y + 4.0f);
        const ImVec2 CheckboxMax(CheckboxMin.x + 12.0f, CheckboxMin.y + 12.0f);
        DrawList->AddRect(CheckboxMin, CheckboxMax, IM_COL32(190, 198, 210, 180), 2.0f);
        if (Group.bVisible)
        {
            DrawList->AddLine(
                ImVec2(CheckboxMin.x + 2.0f, CheckboxMin.y + 6.0f),
                ImVec2(CheckboxMin.x + 5.0f, CheckboxMin.y + 9.0f),
                IM_COL32(98, 202, 255, 255),
                2.0f);
            DrawList->AddLine(
                ImVec2(CheckboxMin.x + 5.0f, CheckboxMin.y + 9.0f),
                ImVec2(CheckboxMax.x - 2.0f, CheckboxMin.y + 2.0f),
                IM_COL32(98, 202, 255, 255),
                2.0f);
        }

        DrawList->AddText(
            ImVec2(CheckboxMax.x + 8.0f, Min.y + 3.0f),
            IM_COL32(215, 220, 228, 255),
            Group.Label.c_str());
        const FString CountLabel = std::to_string(Group.TotalCount);
        const ImVec2 CountSize = ImGui::CalcTextSize(CountLabel.c_str());
        DrawList->AddText(
            ImVec2(Max.x - CountSize.x - 10.0f, Min.y + 3.0f),
            IM_COL32(143, 150, 161, 255),
            CountLabel.c_str());

        ImGui::PopID();
        return bClicked;
    }

    void DrawInfoRow(
        const FString& Label,
        const ImVec2& Min,
        const ImVec2& Max)
    {
        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRectFilled(Min, Max, IM_COL32(24, 27, 33, 205), 4.0f);
        DrawList->AddRect(Min, Max, IM_COL32(255, 255, 255, 14), 4.0f);
        DrawList->AddText(ImVec2(Min.x + 16.0f, Min.y + 4.0f), IM_COL32(132, 139, 150, 255), Label.c_str());
    }
}

void FAnimationSequenceTrackOutlinerWidget::Render(
    FAnimationSequenceEditorState& State,
    FAnimationSequenceEditorDocument* Document,
    const TArray<FAnimationSequenceCurveViewGroup>& CurveGroups,
    const TArray<FAnimationSequenceCurveViewGroup>& AttributeCurveGroups)
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
    // DrawList->AddRectFilled(
    //     ImVec2(CanvasPos.x + 1.0f, HeaderTop),
    //     ImVec2(CanvasEnd.x - 1.0f, HeaderBottom),
    //     IM_COL32(30, 34, 40, 255),
    //     4.0f);
    // DrawList->AddText(ImVec2(CanvasPos.x + 10.0f, HeaderTop + 4.0f), IM_COL32(220, 224, 232, 255), "Tracks");

    const int32 NotifyTrackCount = Document ? Document->GetNotifyTrackCount() : 0;
    const float TrackAreaTop =
        HeaderBottom +
        FAnimationSequenceSequencerLayout::TrackAreaPadding -
        State.SequencerScrollY;
    const float NotifySectionHeight =
        FAnimationSequenceSequencerLayout::GetNotifySectionHeight(NotifyTrackCount, State.bNotifiesExpanded);
    const float CurveSectionTop = TrackAreaTop + NotifySectionHeight + FAnimationSequenceSequencerLayout::SectionGap;
    const float CurveSectionHeight =
        AnimationSequenceCurveFilter::GetCurveSectionHeight(CurveGroups, State.bCurvesExpanded);
    const float AdditiveSectionTop = CurveSectionTop + CurveSectionHeight + FAnimationSequenceSequencerLayout::SectionGap;
    const float AdditiveSectionHeight =
        FAnimationSequenceSequencerLayout::GetPlaceholderSectionHeight(State.bAdditiveLayerTracksExpanded);
    const float AttributeSectionTop = AdditiveSectionTop + AdditiveSectionHeight + FAnimationSequenceSequencerLayout::SectionGap;

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

    if (State.bCurvesExpanded)
    {
        float RowCursorY = CurveSectionTop + FAnimationSequenceSequencerLayout::SectionHeaderHeight + 4.0f;
        for (int32 GroupIndex = 0; GroupIndex < static_cast<int32>(CurveGroups.size()); ++GroupIndex)
        {
            const FAnimationSequenceCurveViewGroup& Group = CurveGroups[GroupIndex];
            const float GroupTop = RowCursorY;
            const float GroupBottom = GroupTop + FAnimationSequenceSequencerLayout::CurveGroupHeaderHeight;

            if (DrawCurveGroupRow(
                    3000 + GroupIndex,
                    Group,
                    ImVec2(CanvasPos.x + 8.0f, GroupTop),
                    ImVec2(CanvasEnd.x - 8.0f, GroupBottom)))
            {
                AnimationSequenceCurveFilter::SetCurveTypeEnabled(State, Group.CurveType, !Group.bVisible);
            }

            RowCursorY = GroupBottom;
            if (Group.bVisible && !Group.VisibleEntries.empty())
            {
                RowCursorY += FAnimationSequenceSequencerLayout::CurveGroupHeaderSpacing;
                for (int32 EntryIndex = 0; EntryIndex < static_cast<int32>(Group.VisibleEntries.size()); ++EntryIndex)
                {
                    const FAnimationSequenceCurveViewEntry& Entry = Group.VisibleEntries[EntryIndex];
                    const float RowTop = RowCursorY;
                    const float RowBottom = RowTop + FAnimationSequenceSequencerLayout::CurveTrackRowHeight;
                    const bool bSelected = State.SelectedCurveIndex == Entry.SourceIndex;

                    if (DrawSelectableRow(
                            4000 + Entry.SourceIndex,
                            Entry.Curve ? Entry.Curve->CurveName.ToString() : FString("Curve"),
                            Entry.Curve ? (std::to_string(static_cast<int32>(Entry.Curve->Keys.size())) + " keys") : FString(),
                            bSelected,
                            ImVec2(CanvasPos.x + 18.0f, RowTop),
                            ImVec2(CanvasEnd.x - 8.0f, RowBottom)))
                    {
                        State.SelectedCurveIndex = Entry.SourceIndex;
                        State.HoveredCurveIndex = Entry.SourceIndex;
                        if (Document)
                        {
                            Document->ClearNotifySelection();
                        }
                    }

                    RowCursorY = RowBottom;
                    if (EntryIndex + 1 < static_cast<int32>(Group.VisibleEntries.size()))
                    {
                        RowCursorY += FAnimationSequenceSequencerLayout::CurveTrackRowSpacing;
                    }
                }
            }

            if (GroupIndex + 1 < static_cast<int32>(CurveGroups.size()))
            {
                RowCursorY += FAnimationSequenceSequencerLayout::CurveGroupHeaderSpacing;
            }
        }
    }

    DrawSectionRow(
        "AdditiveLayerSection",
        "Additive Layer Tracks",
        State.bAdditiveLayerTracksExpanded,
        ImVec2(CanvasPos.x + 4.0f, AdditiveSectionTop),
        ImVec2(CanvasEnd.x - 4.0f, AdditiveSectionTop + FAnimationSequenceSequencerLayout::SectionHeaderHeight));

    if (State.bAdditiveLayerTracksExpanded)
    {
        const float RowTop = AdditiveSectionTop + FAnimationSequenceSequencerLayout::SectionHeaderHeight + 4.0f;
        const float RowBottom = RowTop + FAnimationSequenceSequencerLayout::EmptySectionRowHeight;
        DrawInfoRow(
            "No additive layer tracks.",
            ImVec2(CanvasPos.x + 8.0f, RowTop),
            ImVec2(CanvasEnd.x - 8.0f, RowBottom));
    }

    DrawSectionRow(
        "AttributeSection",
        "Attributes",
        State.bAttributesExpanded,
        ImVec2(CanvasPos.x + 4.0f, AttributeSectionTop),
        ImVec2(CanvasEnd.x - 4.0f, AttributeSectionTop + FAnimationSequenceSequencerLayout::SectionHeaderHeight));

    if (State.bAttributesExpanded)
    {
        float RowCursorY = AttributeSectionTop + FAnimationSequenceSequencerLayout::SectionHeaderHeight + 4.0f;
        if (AttributeCurveGroups.empty())
        {
            const float RowBottom = RowCursorY + FAnimationSequenceSequencerLayout::EmptySectionRowHeight;
            DrawInfoRow(
                "No attribute tracks.",
                ImVec2(CanvasPos.x + 8.0f, RowCursorY),
                ImVec2(CanvasEnd.x - 8.0f, RowBottom));
        }
        else
        {
            for (int32 GroupIndex = 0; GroupIndex < static_cast<int32>(AttributeCurveGroups.size()); ++GroupIndex)
            {
                const FAnimationSequenceCurveViewGroup& Group = AttributeCurveGroups[GroupIndex];
                const float GroupTop = RowCursorY;
                const float GroupBottom = GroupTop + FAnimationSequenceSequencerLayout::CurveGroupHeaderHeight;

                if (DrawCurveGroupRow(
                        5000 + GroupIndex,
                        Group,
                        ImVec2(CanvasPos.x + 8.0f, GroupTop),
                        ImVec2(CanvasEnd.x - 8.0f, GroupBottom)))
                {
                    AnimationSequenceCurveFilter::SetCurveTypeEnabled(State, Group.CurveType, !Group.bVisible);
                }

                RowCursorY = GroupBottom;
                if (Group.bVisible && !Group.VisibleEntries.empty())
                {
                    RowCursorY += FAnimationSequenceSequencerLayout::CurveGroupHeaderSpacing;
                    for (int32 EntryIndex = 0; EntryIndex < static_cast<int32>(Group.VisibleEntries.size()); ++EntryIndex)
                    {
                        const FAnimationSequenceCurveViewEntry& Entry = Group.VisibleEntries[EntryIndex];
                        const float RowTop = RowCursorY;
                        const float RowBottom = RowTop + FAnimationSequenceSequencerLayout::CurveTrackRowHeight;
                        const bool bSelected = State.SelectedCurveIndex == Entry.SourceIndex;

                        if (DrawSelectableRow(
                                6000 + Entry.SourceIndex,
                                Entry.Curve ? Entry.Curve->CurveName.ToString() : FString("Curve"),
                                Entry.Curve ? (std::to_string(static_cast<int32>(Entry.Curve->Keys.size())) + " keys") : FString(),
                                bSelected,
                                ImVec2(CanvasPos.x + 18.0f, RowTop),
                                ImVec2(CanvasEnd.x - 8.0f, RowBottom)))
                        {
                            State.SelectedCurveIndex = Entry.SourceIndex;
                            State.HoveredCurveIndex = Entry.SourceIndex;
                            if (Document)
                            {
                                Document->ClearNotifySelection();
                            }
                        }

                        RowCursorY = RowBottom;
                        if (EntryIndex + 1 < static_cast<int32>(Group.VisibleEntries.size()))
                        {
                            RowCursorY += FAnimationSequenceSequencerLayout::CurveTrackRowSpacing;
                        }
                    }
                }

                if (GroupIndex + 1 < static_cast<int32>(AttributeCurveGroups.size()))
                {
                    RowCursorY += FAnimationSequenceSequencerLayout::CurveGroupHeaderSpacing;
                }
            }
        }
    }
}
