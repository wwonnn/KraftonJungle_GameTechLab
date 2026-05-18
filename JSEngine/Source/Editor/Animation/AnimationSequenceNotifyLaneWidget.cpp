#include "Editor/Animation/AnimationSequenceNotifyLaneWidget.h"

#include "Editor/Animation/AnimationSequenceEditorDocument.h"
#include "Editor/Animation/AnimationSequenceEditorState.h"
#include "Editor/Animation/AnimationSequenceTimelineGeometry.h"

#include "ImGui/imgui.h"

#include <algorithm>

namespace
{
    constexpr float LaneHeaderHeight = 20.0f;
    constexpr float TrackRowHeight = 24.0f;
    constexpr float TrackRowSpacing = 4.0f;
    constexpr float MarkerMinWidth = 12.0f;
    constexpr float MarkerCornerRadius = 4.0f;

    ImU32 WithAlpha(ImU32 Color, float AlphaScale)
    {
        const ImU32 Alpha = static_cast<ImU32>(std::clamp(AlphaScale, 0.0f, 1.0f) * 255.0f);
        return (Color & 0x00ffffffu) | (Alpha << 24);
    }

    float GetLaneHeight(int32 TrackCount)
    {
        const int32 SafeTrackCount = std::max(TrackCount, 1);
        return LaneHeaderHeight + 10.0f + SafeTrackCount * TrackRowHeight + (SafeTrackCount - 1) * TrackRowSpacing + 8.0f;
    }

    ImU32 ToImGuiColor(const FColor& Color, float AlphaScale = 1.0f)
    {
        return ImGui::GetColorU32(ImVec4(Color.R, Color.G, Color.B, Color.A * AlphaScale));
    }
}

void FAnimationSequenceNotifyLaneWidget::Render(FAnimationSequenceEditorState& State, FAnimationSequenceEditorDocument* Document)
{
    State.HoveredNotifyTrackIndex = -1;
    State.HoveredNotifyEventIndex = -1;
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        State.bDraggingNotify = false;
        State.DraggedNotifyTrackIndex = -1;
        State.DraggedNotifyEventIndex = -1;
    }

    const int32 TrackCount = Document ? Document->GetNotifyTrackCount() : 0;
    const float NotifyLaneHeight = GetLaneHeight(TrackCount);

    ImGui::BeginChild(
        "##AnimationSequenceNotifyLane",
        ImVec2(0.0f, NotifyLaneHeight),
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
    const ImVec2 CanvasSize = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("##AnimationSequenceNotifyLaneCanvas", CanvasSize);
    const bool bCanvasHovered = ImGui::IsItemHovered();
    const FAnimationSequenceTimelineGeometry Geometry =
        FAnimationSequenceTimelineGeometry::BuildLaneGeometry(CanvasPos, CanvasSize);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    const ImU32 BorderColor = ImGui::GetColorU32(ImGuiCol_Border);
    const ImU32 BackgroundColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
    const ImU32 GridColor = WithAlpha(ImGui::GetColorU32(ImGuiCol_TextDisabled), 0.20f);
    const ImU32 MinorGridColor = WithAlpha(ImGui::GetColorU32(ImGuiCol_TextDisabled), 0.10f);
    const ImU32 LabelColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    const ImU32 PlayheadColor = IM_COL32(255, 106, 72, 255);

    DrawList->AddRectFilled(CanvasPos, Geometry.CanvasEnd, ImGui::GetColorU32(ImGuiCol_ChildBg));
    DrawList->AddRect(CanvasPos, Geometry.CanvasEnd, BorderColor);
    DrawList->AddText(ImVec2(CanvasPos.x + 8.0f, CanvasPos.y + 4.0f), LabelColor, "Notify Lane");

    const bool bHasTimelineData = State.HasTimelineData();
    const int32 MajorStep = bHasTimelineData ? State.ChooseMajorFrameStep(Geometry.TimelineWidth) : 1;
    const int32 MinorStep = bHasTimelineData ? State.ChooseMinorFrameStep(Geometry.TimelineWidth) : 1;
    const int32 EndFrame = bHasTimelineData ? Geometry.GetEndFrame(State) : 0;
    const int32 FirstMinorFrame = bHasTimelineData ? Geometry.GetFirstMinorFrame(State, MinorStep) : 0;
    const ImVec2 MousePos = ImGui::GetIO().MousePos;

    const int32 VisibleTrackCount = std::max(TrackCount, 1);
    bool bHoveredAnyMarker = false;

    for (int32 TrackIndex = 0; TrackIndex < VisibleTrackCount; ++TrackIndex)
    {
        const float RowTop = CanvasPos.y + LaneHeaderHeight + 6.0f + TrackIndex * (TrackRowHeight + TrackRowSpacing);
        const float RowBottom = RowTop + TrackRowHeight;
        const ImVec2 RowMin(Geometry.TimelineMinX, RowTop);
        const ImVec2 RowMax(Geometry.TimelineMaxX, RowBottom);

        DrawList->AddRectFilled(RowMin, RowMax, BackgroundColor, 4.0f);
        DrawList->AddRect(RowMin, RowMax, WithAlpha(BorderColor, 0.70f), 4.0f);

        if (bHasTimelineData)
        {
            DrawList->PushClipRect(RowMin, RowMax, true);
            for (int32 Frame = FirstMinorFrame; Frame <= EndFrame + MinorStep; Frame += std::max(MinorStep, 1))
            {
                const float X = Geometry.TimeToX(State, State.FrameToTime(Frame));
                if (X < Geometry.TimelineMinX - 1.0f || X > Geometry.TimelineMaxX + 1.0f)
                {
                    continue;
                }

                const bool bMajorTick = MajorStep > 0 && (Frame % MajorStep) == 0;
                DrawList->AddLine(
                    ImVec2(X, RowTop),
                    ImVec2(X, RowBottom),
                    bMajorTick ? GridColor : MinorGridColor);
            }

            const float PlayheadX = Geometry.GetClampedPlayheadX(State);
            DrawList->AddLine(ImVec2(PlayheadX, RowTop), ImVec2(PlayheadX, RowBottom), PlayheadColor, 2.0f);
            DrawList->PopClipRect();
        }

        const FString TrackLabel = Document && Document->GetNotifyTrack(TrackIndex) && Document->GetNotifyTrack(TrackIndex)->TrackName.IsValid()
            ? Document->GetNotifyTrack(TrackIndex)->TrackName.ToString()
            : ("Track " + std::to_string(TrackIndex + 1));
        DrawList->AddText(ImVec2(CanvasPos.x + 8.0f, RowTop + 4.0f), LabelColor, TrackLabel.c_str());

        const FAnimNotifyTrack* Track = Document ? Document->GetNotifyTrack(TrackIndex) : nullptr;
        if (!Track)
        {
            continue;
        }

        for (int32 EventIndex = 0; EventIndex < static_cast<int32>(Track->Events.size()); ++EventIndex)
        {
            const FAnimNotifyEvent& NotifyEvent = Track->Events[EventIndex];
            const float MarkerStartX = Geometry.TimeToX(State, NotifyEvent.Time);
            const float MarkerEndTime = NotifyEvent.Time + std::max(NotifyEvent.Duration, 0.0f);
            float MarkerEndX = Geometry.TimeToX(State, MarkerEndTime);
            if (MarkerEndX < MarkerStartX + MarkerMinWidth)
            {
                MarkerEndX = MarkerStartX + MarkerMinWidth;
            }

            const ImVec2 MarkerMin(
                std::clamp(MarkerStartX - 4.0f, Geometry.TimelineMinX, Geometry.TimelineMaxX),
                RowTop + 3.0f);
            const ImVec2 MarkerMax(
                std::clamp(MarkerEndX, Geometry.TimelineMinX + MarkerMinWidth, Geometry.TimelineMaxX),
                RowBottom - 3.0f);

            if (MarkerMax.x <= Geometry.TimelineMinX || MarkerMin.x >= Geometry.TimelineMaxX)
            {
                continue;
            }

            ImGui::SetCursorScreenPos(MarkerMin);
            ImGui::PushID(TrackIndex * 1024 + EventIndex);
            ImGui::InvisibleButton(
                "##AnimNotifyEvent",
                ImVec2(std::max(MarkerMax.x - MarkerMin.x, 1.0f), std::max(MarkerMax.y - MarkerMin.y, 1.0f)));

            const bool bHovered = ImGui::IsItemHovered();
            const bool bActive = ImGui::IsItemActive();
            const bool bSelected =
                State.SelectedNotifyTrackIndex == TrackIndex &&
                State.SelectedNotifyEventIndex == EventIndex;

            if (bHovered)
            {
                State.HoveredNotifyTrackIndex = TrackIndex;
                State.HoveredNotifyEventIndex = EventIndex;
                bHoveredAnyMarker = true;
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && Document)
            {
                Document->SelectNotify(TrackIndex, EventIndex);
            }

            if (Document && bActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                State.bDraggingNotify = true;
                State.DraggedNotifyTrackIndex = TrackIndex;
                State.DraggedNotifyEventIndex = EventIndex;
                Document->SelectNotify(TrackIndex, EventIndex);
            }

            const ImU32 FillColor = bSelected
                ? ToImGuiColor(NotifyEvent.Color, 1.0f)
                : ToImGuiColor(NotifyEvent.Color, bHovered ? 0.95f : 0.78f);
            const ImU32 OutlineColor = bSelected ? IM_COL32(255, 244, 214, 255) : WithAlpha(BorderColor, 0.9f);
            DrawList->AddRectFilled(MarkerMin, MarkerMax, FillColor, MarkerCornerRadius);
            DrawList->AddRect(MarkerMin, MarkerMax, OutlineColor, MarkerCornerRadius, 0, bSelected ? 2.0f : 1.0f);
            DrawList->AddText(
                ImVec2(MarkerMin.x + 5.0f, MarkerMin.y + 1.0f),
                IM_COL32(25, 25, 25, 255),
                NotifyEvent.Name.IsValid() ? NotifyEvent.Name.ToString().c_str() : "Notify");

            ImGui::PopID();
        }
    }

    if (Document &&
        State.bDraggingNotify &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        Document->GetSelectedNotify())
    {
        const float NewTime = State.ClampOrSnapTime(
            Geometry.XToTime(State, std::clamp(MousePos.x, Geometry.TimelineMinX, Geometry.TimelineMaxX)));
        Document->SetSelectedNotifyTime(NewTime, State.bSnapToFrames);
    }

    if (Document && bHasTimelineData && bCanvasHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !bHoveredAnyMarker)
    {
        int32 TargetTrackIndex = 0;
        for (int32 TrackIndex = 0; TrackIndex < VisibleTrackCount; ++TrackIndex)
        {
            const float RowTop = CanvasPos.y + LaneHeaderHeight + 6.0f + TrackIndex * (TrackRowHeight + TrackRowSpacing);
            const float RowBottom = RowTop + TrackRowHeight;
            if (MousePos.y >= RowTop && MousePos.y <= RowBottom)
            {
                TargetTrackIndex = TrackIndex;
                break;
            }
        }

        const float NewTime = State.ClampOrSnapTime(
            Geometry.XToTime(State, std::clamp(MousePos.x, Geometry.TimelineMinX, Geometry.TimelineMaxX)));
        Document->AddNotifyAtTime(TargetTrackIndex, NewTime);
    }

    if (TrackCount <= 0)
    {
        DrawList->AddText(
            ImVec2(CanvasPos.x + 8.0f, CanvasPos.y + LaneHeaderHeight + 9.0f),
            LabelColor,
            "Double-click the lane to add the first notify.");
    }
    else if (!Document || !Document->GetSelectedNotify())
    {
        DrawList->AddText(
            ImVec2(CanvasPos.x + 8.0f, Geometry.CanvasEnd.y - 18.0f),
            LabelColor,
            "Double-click to add a notify. Drag a marker to move it.");
    }

    ImGui::EndChild();
}
