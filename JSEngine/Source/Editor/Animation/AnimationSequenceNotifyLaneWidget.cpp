#include "Editor/Animation/AnimationSequenceNotifyLaneWidget.h"

#include "Editor/Animation/AnimationSequenceEditorState.h"

#include "ImGui/imgui.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float NotifyLaneHeight = 58.0f;
    constexpr float TimelineHorizontalPadding = 14.0f;
    constexpr float TimelineVerticalPadding = 8.0f;

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

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    const ImVec2 CanvasEnd(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y);
    const float TimelineMinX = CanvasPos.x + TimelineHorizontalPadding;
    const float TimelineMaxX = CanvasEnd.x - TimelineHorizontalPadding;
    const float TimelineWidth = std::max(1.0f, TimelineMaxX - TimelineMinX);
    const float LaneTop = CanvasPos.y + TimelineVerticalPadding;
    const float LaneBottom = CanvasEnd.y - TimelineVerticalPadding;

    const ImU32 BorderColor = ImGui::GetColorU32(ImGuiCol_Border);
    const ImU32 BackgroundColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
    const ImU32 GridColor = WithAlpha(ImGui::GetColorU32(ImGuiCol_TextDisabled), 0.20f);
    const ImU32 LabelColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);

    DrawList->AddRectFilled(CanvasPos, CanvasEnd, ImGui::GetColorU32(ImGuiCol_ChildBg));
    DrawList->AddRectFilled(ImVec2(TimelineMinX, LaneTop), ImVec2(TimelineMaxX, LaneBottom), BackgroundColor, 4.0f);
    DrawList->AddRect(CanvasPos, CanvasEnd, BorderColor);

    if (State.HasTimelineData())
    {
        const auto TimeToX = [&](float Time)
        {
            return TimelineMinX + ((Time - State.VisibleTimeStart) / State.GetVisibleRange()) * TimelineWidth;
        };

        const int32 MajorStep = State.ChooseMajorFrameStep(TimelineWidth);
        const int32 MinorStep = State.ChooseMinorFrameStep(TimelineWidth);
        const int32 StartFrame = std::max(0, static_cast<int32>(std::floor(State.VisibleTimeStart * State.GetDisplayFPS())));
        const int32 EndFrame = std::max(StartFrame, static_cast<int32>(std::ceil(State.VisibleTimeEnd * State.GetDisplayFPS())));
        const int32 FirstMinorFrame = MinorStep > 0 ? (StartFrame / MinorStep) * MinorStep : StartFrame;

        DrawList->PushClipRect(ImVec2(TimelineMinX, LaneTop), ImVec2(TimelineMaxX, LaneBottom), true);
        for (int32 Frame = FirstMinorFrame; Frame <= EndFrame + MinorStep; Frame += std::max(MinorStep, 1))
        {
            const float X = TimeToX(State.FrameToTime(Frame));
            if (X < TimelineMinX - 1.0f || X > TimelineMaxX + 1.0f)
            {
                continue;
            }

            const bool bMajorTick = MajorStep > 0 && (Frame % MajorStep) == 0;
            DrawList->AddLine(
                ImVec2(X, LaneTop),
                ImVec2(X, LaneBottom),
                bMajorTick ? GridColor : WithAlpha(GridColor, 0.55f));
        }

        const float PlayheadX = std::clamp(TimeToX(State.CurrentTime), TimelineMinX, TimelineMaxX);
        DrawList->AddLine(ImVec2(PlayheadX, LaneTop), ImVec2(PlayheadX, LaneBottom), IM_COL32(255, 106, 72, 255), 2.0f);
        DrawList->PopClipRect();
    }

    DrawList->AddText(ImVec2(CanvasPos.x + 8.0f, CanvasPos.y + 6.0f), LabelColor, "Notify Lane");
    DrawList->AddText(
        ImVec2(CanvasPos.x + 8.0f, CanvasPos.y + 26.0f),
        LabelColor,
        "Anim Notify data/model is not implemented yet. This lane is reserved for Priority 3.");

    ImGui::EndChild();
}
