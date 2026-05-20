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
    constexpr float NotifyStateResizeHandleWidth = 8.0f;

    ImU32 ToImGuiColor(const FColor& Color, float AlphaScale)
    {
        const float Alpha = std::clamp(Color.A * AlphaScale, 0.0f, 1.0f);
        return ImGui::GetColorU32(ImVec4(Color.R, Color.G, Color.B, Alpha));
    }

    enum class EPendingNotifyContextAction : uint8
    {
        None,
        Duplicate,
        Copy,
        Delete,
        MoveToTrack
    };
}

void FAnimationSequenceNotifyLaneWidget::RenderRows(
    FAnimationSequenceEditorState& State,
    FAnimationSequenceEditorDocument* Document,
    const FAnimationSequenceTimelineGeometry& Geometry,
    const FAnimationSequenceSequencerVisibleLayout& VisibleLayout,
    float TimelineRowOriginY)
{
    static int32 PendingLaneContextTrackIndex = -1;
    static float PendingLaneContextTime = 0.0f;
    EPendingNotifyContextAction PendingContextAction = EPendingNotifyContextAction::None;
    int32 PendingMoveTargetTrackIndex = -1;

    State.HoveredNotifyTrackIndex = -1;
    State.HoveredNotifyEventIndex = -1;
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        State.bDraggingNotify = false;
        State.ActiveNotifyDragMode = EAnimNotifyDragMode::None;
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
            const bool bCanResizeEnd = NotifyEvent.IsState();
            const float ResizeHandleMinX = std::max(MarkerMin.x, MarkerMax.x - NotifyStateResizeHandleWidth);
            const bool bHoveredResizeHandle =
                bCanResizeEnd &&
                bHovered &&
                ImGui::GetIO().MousePos.x >= ResizeHandleMinX &&
                ImGui::GetIO().MousePos.x <= MarkerMax.x;
            if (bHovered)
            {
                bMouseOverMarker = true;
                State.HoveredNotifyTrackIndex = Row.SourceIndex;
                State.HoveredNotifyEventIndex = EventIndex;
                State.SetHoveredSequencerRow(Row.Id);
                if (bHoveredResizeHandle)
                {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                }
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && Document)
            {
                State.SetFocusedSequencerRow(Row.Id);
                State.SelectedCurveIndex = -1;
                State.HoveredCurveIndex = -1;
                State.ActiveNotifyDragMode =
                    bHoveredResizeHandle ? EAnimNotifyDragMode::ResizeEnd : EAnimNotifyDragMode::Move;
                State.DraggedNotifyGrabOffsetTime = State.ActiveNotifyDragMode == EAnimNotifyDragMode::Move
                    ? (Geometry.XToTime(State, Geometry.ClampX(ImGui::GetIO().MousePos.x)) - NotifyEvent.Time)
                    : 0.0f;
                Document->SelectNotify(Row.SourceIndex, EventIndex);
            }

            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                State.bDraggingNotify = true;
                State.DraggedNotifyTrackIndex = Row.SourceIndex;
                State.DraggedNotifyEventIndex = EventIndex;
                State.SelectedCurveIndex = -1;
                if (State.ActiveNotifyDragMode == EAnimNotifyDragMode::None)
                {
                    State.ActiveNotifyDragMode = EAnimNotifyDragMode::Move;
                }
                if (Document)
                {
                    Document->SelectNotify(Row.SourceIndex, EventIndex);
                }
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && Document)
            {
                State.SetFocusedSequencerRow(Row.Id);
                Document->SelectNotify(Row.SourceIndex, EventIndex);
            }

            if (Document && ImGui::BeginPopupContextItem("##AnimNotifyEventContext"))
            {
                if (ImGui::MenuItem("Duplicate"))
                {
                    PendingContextAction = EPendingNotifyContextAction::Duplicate;
                }
                if (ImGui::MenuItem("Copy"))
                {
                    PendingContextAction = EPendingNotifyContextAction::Copy;
                }
                if (ImGui::MenuItem("Delete"))
                {
                    PendingContextAction = EPendingNotifyContextAction::Delete;
                }

                if (ImGui::BeginMenu("Move To Track"))
                {
                    const int32 TrackCount = Document->GetNotifyTrackCount();
                    for (int32 TrackIndex = 0; TrackIndex < TrackCount; ++TrackIndex)
                    {
                        const FAnimNotifyTrack* TargetTrack = Document->GetNotifyTrack(TrackIndex);
                        const FString TrackLabel =
                            (TargetTrack && TargetTrack->TrackName.IsValid())
                                ? TargetTrack->TrackName.ToString()
                                : ("Track " + std::to_string(TrackIndex + 1));
                        const bool bIsCurrentTrack = TrackIndex == Row.SourceIndex;
                        if (ImGui::MenuItem(TrackLabel.c_str(), nullptr, false, !bIsCurrentTrack))
                        {
                            PendingContextAction = EPendingNotifyContextAction::MoveToTrack;
                            PendingMoveTargetTrackIndex = TrackIndex;
                        }
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndPopup();
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

    if (Document)
    {
        switch (PendingContextAction)
        {
        case EPendingNotifyContextAction::Duplicate:
            Document->DuplicateSelectedNotify();
            break;
        case EPendingNotifyContextAction::Copy:
            Document->CopySelectedNotify();
            break;
        case EPendingNotifyContextAction::Delete:
            Document->DeleteSelectedNotify();
            break;
        case EPendingNotifyContextAction::MoveToTrack:
            if (Document->MoveSelectedNotifyToTrack(PendingMoveTargetTrackIndex))
            {
                State.SetFocusedSequencerRow(
                    MakeAnimationSequenceSequencerRowId(
                        EAnimationSequenceSequencerVisibleRowType::NotifyTrack,
                        PendingMoveTargetTrackIndex));
            }
            break;
        case EPendingNotifyContextAction::None:
        default:
            break;
        }
    }

    if (State.bDraggingNotify && ImGui::IsMouseDown(ImGuiMouseButton_Left) && Document && Document->GetSelectedNotify())
    {
        const float MouseTime = Geometry.XToTime(State, Geometry.ClampX(ImGui::GetIO().MousePos.x));
        if (State.ActiveNotifyDragMode == EAnimNotifyDragMode::ResizeEnd)
        {
            Document->SetSelectedNotifyEndTime(MouseTime, State.bSnapToFrames);
        }
        else
        {
            const float NewTime = MouseTime - State.DraggedNotifyGrabOffsetTime;
            Document->SetSelectedNotifyTime(NewTime, State.bSnapToFrames);
        }
    }

    if (Document && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !bMouseOverMarker)
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
            if (MousePos.y >= RowTop && MousePos.y <= RowBottom &&
                MousePos.x >= Geometry.TimelineMinX && MousePos.x <= Geometry.TimelineMaxX)
            {
                PendingLaneContextTrackIndex = Row.SourceIndex;
                PendingLaneContextTime = Geometry.XToTime(State, Geometry.ClampX(MousePos.x));
                State.SetFocusedSequencerRow(Row.Id);
                ImGui::OpenPopup("##AnimNotifyLaneContext");
                break;
            }
        }
    }

    if (Document && ImGui::BeginPopup("##AnimNotifyLaneContext"))
    {
        if (ImGui::MenuItem("Add Notify Here", nullptr, false, PendingLaneContextTrackIndex >= 0))
        {
            Document->AddNotifyAtTime(PendingLaneContextTrackIndex, PendingLaneContextTime);
        }

        if (ImGui::MenuItem("Paste Notify Here", nullptr, false, PendingLaneContextTrackIndex >= 0 && Document->CanPasteNotify()))
        {
            Document->PasteNotifyToTrackAtTime(PendingLaneContextTrackIndex, PendingLaneContextTime);
        }

        ImGui::EndPopup();
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
