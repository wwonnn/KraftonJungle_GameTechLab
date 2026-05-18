#include "Editor/Animation/AnimationSequenceNotifyLaneWidget.h"

#include "Editor/Animation/AnimationSequenceEditorState.h"
#include "Editor/Animation/AnimationSequenceTimelineGeometry.h"

#include "ImGui/imgui.h"

#include <algorithm>

namespace
{
    constexpr float NotifyLaneHeight = 58.0f;

    ImU32 WithAlpha(ImU32 Color, float AlphaScale)
    {
        const ImU32 Alpha = static_cast<ImU32>(std::clamp(AlphaScale, 0.0f, 1.0f) * 255.0f);
        return (Color & 0x00ffffffu) | (Alpha << 24);
    }
}

void FAnimationSequenceNotifyLaneWidget::Render(FAnimationSequenceEditorState& State)
{
    State.HoveredNotifyIndex = -1;

    ImGui::BeginChild(
        "##AnimationSequenceNotifyLane",
        ImVec2(0.0f, NotifyLaneHeight),
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
    const ImVec2 CanvasSize = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("##AnimationSequenceNotifyLaneCanvas", CanvasSize);
    const FAnimationSequenceTimelineGeometry Geometry =
        FAnimationSequenceTimelineGeometry::BuildLaneGeometry(CanvasPos, CanvasSize);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImU32 BorderColor = ImGui::GetColorU32(ImGuiCol_Border);
    const ImU32 BackgroundColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
    const ImU32 GridColor = WithAlpha(ImGui::GetColorU32(ImGuiCol_TextDisabled), 0.20f);
    const ImU32 LabelColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);

    DrawList->AddRectFilled(CanvasPos, Geometry.CanvasEnd, ImGui::GetColorU32(ImGuiCol_ChildBg));
    DrawList->AddRectFilled(
        ImVec2(Geometry.TimelineMinX, Geometry.LaneTop),
        ImVec2(Geometry.TimelineMaxX, Geometry.LaneBottom),
        BackgroundColor,
        4.0f);
    DrawList->AddRect(CanvasPos, Geometry.CanvasEnd, BorderColor);

    if (State.HasTimelineData())
    {
        const int32 MajorStep = State.ChooseMajorFrameStep(Geometry.TimelineWidth);
        const int32 MinorStep = State.ChooseMinorFrameStep(Geometry.TimelineWidth);
        const int32 EndFrame = Geometry.GetEndFrame(State);
        const int32 FirstMinorFrame = Geometry.GetFirstMinorFrame(State, MinorStep);

        DrawList->PushClipRect(
            ImVec2(Geometry.TimelineMinX, Geometry.LaneTop),
            ImVec2(Geometry.TimelineMaxX, Geometry.LaneBottom),
            true);
        for (int32 Frame = FirstMinorFrame; Frame <= EndFrame + MinorStep; Frame += std::max(MinorStep, 1))
        {
            const float X = Geometry.TimeToX(State, State.FrameToTime(Frame));
            if (X < Geometry.TimelineMinX - 1.0f || X > Geometry.TimelineMaxX + 1.0f)
            {
                continue;
            }

            const bool bMajorTick = MajorStep > 0 && (Frame % MajorStep) == 0;
            DrawList->AddLine(
                ImVec2(X, Geometry.LaneTop),
                ImVec2(X, Geometry.LaneBottom),
                bMajorTick ? GridColor : WithAlpha(GridColor, 0.55f));
        }

        const float PlayheadX = Geometry.GetClampedPlayheadX(State);
        DrawList->AddLine(
            ImVec2(PlayheadX, Geometry.LaneTop),
            ImVec2(PlayheadX, Geometry.LaneBottom),
            IM_COL32(255, 106, 72, 255),
            2.0f);
        DrawList->PopClipRect();
    }

    DrawList->AddText(ImVec2(CanvasPos.x + 8.0f, CanvasPos.y + 6.0f), LabelColor, "Notify Lane");
    DrawList->AddText(
        ImVec2(CanvasPos.x + 8.0f, CanvasPos.y + 26.0f),
        LabelColor,
        "Anim Notify data/model is not implemented yet. This lane is reserved for Priority 3.");

    ImGui::EndChild();
}
