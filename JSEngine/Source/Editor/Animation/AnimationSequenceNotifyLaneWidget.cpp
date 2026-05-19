#include "Editor/Animation/AnimationSequenceNotifyLaneWidget.h"

#include "Animation/AnimData/AnimNotifyTypes.h"
#include "Editor/Animation/AnimationSequenceEditorDocument.h"
#include "Editor/Animation/AnimationSequenceEditorState.h"
#include "Editor/Animation/AnimationSequenceSequencerLayout.h"
#include "Editor/Animation/AnimationSequenceSequencerVisibleLayout.h"
#include "Editor/Animation/AnimationSequenceTimelineGeometry.h"

#include "ImGui/imgui.h"

#include <algorithm>

namespace
{
    ImU32 ToImGuiColor(const FColor& Color, float AlphaScale)
    {
        const float Alpha = std::clamp(Color.A * AlphaScale, 0.0f, 1.0f);
        return ImGui::GetColorU32(ImVec4(Color.R, Color.G, Color.B, Alpha));
    }
}

void FAnimationSequenceNotifyLaneWidget::RenderRows(
    FAnimationSequenceEditorState& State,
    FAnimationSequenceEditorDocument* Document,
    const FAnimationSequenceTimelineGeometry& Geometry,
    const FAnimationSequenceSequencerVisibleLayout& VisibleLayout,
    float TimelineRowOriginY)
{
    State.HoveredNotifyTrackIndex = -1;
    State.HoveredNotifyEventIndex = -1;
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        State.bDraggingNotify = false;
        State.DraggedNotifyTrackIndex = -1;
        State.DraggedNotifyEventIndex = -1;
        State.DraggedNotifyGrabOffsetTime = 0.0f;
    }

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    bool bMouseOverMarker = false;

    for (const FAnimationSequenceSequencerVisibleRow& Row : VisibleLayout.Rows)
    {
        if (Row.Type != EAnimationSequenceSequencerVisibleRowType::NotifyTrack)
        {
            continue;
        }

        const float RowTop = TimelineRowOriginY + Row.OffsetY;
        const float RowBottom = RowTop + Row.Height;
        const FAnimNotifyTrack* Track = Document ? Document->GetNotifyTrack(Row.SourceIndex) : nullptr;
        if (!Track)
        {
            continue;
        }

        for (int32 EventIndex = 0; EventIndex < static_cast<int32>(Track->Events.size()); ++EventIndex)
        {
            const FAnimNotifyEvent& NotifyEvent = Track->Events[EventIndex];
            const float MarkerStartX = Geometry.TimeToX(State, NotifyEvent.Time);
            const float MarkerEndTime = NotifyEvent.Time + std::max(NotifyEvent.Duration, 0.0f);
            const float MarkerEndX = Geometry.TimeToX(State, MarkerEndTime);
            const float MarkerWidth = std::max(MarkerEndX - MarkerStartX, 12.0f);
            const ImVec2 MarkerMin(
                std::clamp(MarkerStartX, Geometry.TimelineMinX, Geometry.TimelineMaxX - 12.0f),
                RowTop + 3.0f);
            const ImVec2 MarkerMax(
                std::clamp(MarkerMin.x + MarkerWidth, MarkerMin.x + 12.0f, Geometry.TimelineMaxX),
                RowBottom - 3.0f);

            ImGui::SetCursorScreenPos(MarkerMin);
            ImGui::PushID(Row.SourceIndex * 1024 + EventIndex);
            ImGui::InvisibleButton(
                "##AnimNotifyEvent",
                ImVec2(std::max(MarkerMax.x - MarkerMin.x, 1.0f), std::max(MarkerMax.y - MarkerMin.y, 1.0f)));

            const bool bSelected =
                State.SelectedNotifyTrackIndex == Row.SourceIndex &&
                State.SelectedNotifyEventIndex == EventIndex;
            const bool bHovered = ImGui::IsItemHovered();
            if (bHovered)
            {
                bMouseOverMarker = true;
                State.HoveredNotifyTrackIndex = Row.SourceIndex;
                State.HoveredNotifyEventIndex = EventIndex;
                State.SetHoveredSequencerRow(Row.Id);
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && Document)
            {
                State.SetFocusedSequencerRow(Row.Id);
                State.SelectedCurveIndex = -1;
                State.HoveredCurveIndex = -1;
                State.DraggedNotifyGrabOffsetTime =
                    Geometry.XToTime(State, Geometry.ClampX(ImGui::GetIO().MousePos.x)) - NotifyEvent.Time;
                Document->SelectNotify(Row.SourceIndex, EventIndex);
            }

            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                State.bDraggingNotify = true;
                State.DraggedNotifyTrackIndex = Row.SourceIndex;
                State.DraggedNotifyEventIndex = EventIndex;
                State.SelectedCurveIndex = -1;
                if (Document)
                {
                    Document->SelectNotify(Row.SourceIndex, EventIndex);
                }
            }

            const ImU32 MarkerColor = bSelected
                ? ToImGuiColor(NotifyEvent.Color, 1.0f)
                : ToImGuiColor(NotifyEvent.Color, bHovered ? 0.95f : 0.78f);
            DrawList->AddRectFilled(MarkerMin, MarkerMax, MarkerColor, 4.0f);
            DrawList->AddRect(MarkerMin, MarkerMax, bSelected ? IM_COL32(255, 255, 255, 255) : IM_COL32(0, 0, 0, 96), 4.0f);
            DrawList->AddText(
                ImVec2(MarkerMin.x + 6.0f, MarkerMin.y + 2.0f),
                IM_COL32(14, 18, 22, 255),
                NotifyEvent.Name.IsValid() ? NotifyEvent.Name.ToString().c_str() : "Notify");
            ImGui::PopID();
        }
    }

    if (State.bDraggingNotify && ImGui::IsMouseDown(ImGuiMouseButton_Left) && Document && Document->GetSelectedNotify())
    {
        const float NewTime =
            Geometry.XToTime(State, Geometry.ClampX(ImGui::GetIO().MousePos.x)) - State.DraggedNotifyGrabOffsetTime;
        Document->SetSelectedNotifyTime(NewTime, State.bSnapToFrames);
    }

    if (Document && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !bMouseOverMarker)
    {
        const ImVec2 MousePos = ImGui::GetIO().MousePos;
        for (const FAnimationSequenceSequencerVisibleRow& Row : VisibleLayout.Rows)
        {
            if (Row.Type != EAnimationSequenceSequencerVisibleRowType::NotifyTrack)
            {
                continue;
            }

            const float RowTop = TimelineRowOriginY + Row.OffsetY;
            const float RowBottom = RowTop + Row.Height;
            if (MousePos.y >= RowTop && MousePos.y <= RowBottom)
            {
                const float NewTime = Geometry.XToTime(State, Geometry.ClampX(MousePos.x));
                State.SetFocusedSequencerRow(Row.Id);
                State.SelectedCurveIndex = -1;
                Document->AddNotifyAtTime(Row.SourceIndex, NewTime);
                break;
            }
        }
    }
}
