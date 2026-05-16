#include "Editor/Animation/AnimationSequenceTimelineWidget.h"

#include "Editor/Animation/AnimationSequenceEditorState.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"

#include "ImGui/imgui.h"

#include <algorithm>
#include <cstdio>
#include <cmath>

namespace
{
    constexpr float TimelineHeight = 92.0f;
    constexpr float TimelineHorizontalPadding = 14.0f;
    constexpr float TimelineVerticalPadding = 8.0f;
    constexpr float RulerHeight = 24.0f;

    ImU32 WithAlpha(ImU32 Color, float AlphaScale)
    {
        const ImU32 Alpha = static_cast<ImU32>(std::clamp(AlphaScale, 0.0f, 1.0f) * 255.0f);
        return (Color & 0x00ffffffu) | (Alpha << 24);
    }
}

void FAnimationSequenceTimelineWidget::Render(
    FAnimationSequenceEditorState& State,
    FAnimationSequencePreviewController* PreviewController)
{
    ImGui::BeginChild(
        "##AnimationSequenceTimeline",
        ImVec2(0.0f, TimelineHeight),
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
    const ImVec2 CanvasSize = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("##AnimationSequenceTimelineCanvas", CanvasSize);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    const ImVec2 CanvasEnd(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y);
    const float TimelineMinX = CanvasPos.x + TimelineHorizontalPadding;
    const float TimelineMaxX = CanvasEnd.x - TimelineHorizontalPadding;
    const float TimelineWidth = std::max(1.0f, TimelineMaxX - TimelineMinX);
    const float RulerTop = CanvasPos.y + TimelineVerticalPadding;
    const float TrackTop = RulerTop + RulerHeight;

    const ImU32 BorderColor = ImGui::GetColorU32(ImGuiCol_Border);
    const ImU32 PanelColor = ImGui::GetColorU32(ImGuiCol_ChildBg);
    const ImU32 RulerColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
    const ImU32 GridColor = WithAlpha(ImGui::GetColorU32(ImGuiCol_TextDisabled), 0.34f);
    const ImU32 MinorGridColor = WithAlpha(ImGui::GetColorU32(ImGuiCol_TextDisabled), 0.16f);
    const ImU32 PlayheadColor = IM_COL32(255, 106, 72, 255);
    const ImU32 AccentColor = IM_COL32(255, 170, 72, 255);

    DrawList->AddRectFilled(CanvasPos, CanvasEnd, PanelColor);
    DrawList->AddRectFilled(
        ImVec2(TimelineMinX, RulerTop),
        ImVec2(TimelineMaxX, CanvasEnd.y - TimelineVerticalPadding),
        RulerColor,
        4.0f);
    DrawList->AddRect(CanvasPos, CanvasEnd, BorderColor);
    DrawList->AddLine(ImVec2(TimelineMinX, TrackTop), ImVec2(TimelineMaxX, TrackTop), BorderColor);

    if (!State.HasTimelineData())
    {
        DrawList->AddText(
            ImVec2(TimelineMinX, TrackTop + 10.0f),
            ImGui::GetColorU32(ImGuiCol_TextDisabled),
            "Timeline data is unavailable.");
        ImGui::EndChild();
        return;
    }

    const auto TimeToX = [&](float Time)
    {
        return TimelineMinX + ((Time - State.VisibleTimeStart) / State.GetVisibleRange()) * TimelineWidth;
    };
    const auto XToTime = [&](float X)
    {
        const float Alpha = (X - TimelineMinX) / TimelineWidth;
        return State.VisibleTimeStart + Alpha * State.GetVisibleRange();
    };

    const int32 MajorStep = State.ChooseMajorFrameStep(TimelineWidth);
    const int32 MinorStep = State.ChooseMinorFrameStep(TimelineWidth);
    const int32 StartFrame = std::max(0, static_cast<int32>(std::floor(State.VisibleTimeStart * State.GetDisplayFPS())));
    const int32 EndFrame = std::max(StartFrame, static_cast<int32>(std::ceil(State.VisibleTimeEnd * State.GetDisplayFPS())));
    const int32 FirstMinorFrame = MinorStep > 0 ? (StartFrame / MinorStep) * MinorStep : StartFrame;

    DrawList->PushClipRect(ImVec2(TimelineMinX, RulerTop), ImVec2(TimelineMaxX, CanvasEnd.y - TimelineVerticalPadding), true);

    for (int32 Frame = FirstMinorFrame; Frame <= EndFrame + MinorStep; Frame += std::max(MinorStep, 1))
    {
        const float Time = State.FrameToTime(Frame);
        const float X = TimeToX(Time);
        if (X < TimelineMinX - 1.0f || X > TimelineMaxX + 1.0f)
        {
            continue;
        }

        const bool bMajorTick = MajorStep > 0 && (Frame % MajorStep) == 0;
        DrawList->AddLine(
            ImVec2(X, bMajorTick ? RulerTop : TrackTop - 10.0f),
            ImVec2(X, CanvasEnd.y - TimelineVerticalPadding),
            bMajorTick ? GridColor : MinorGridColor);

        if (bMajorTick)
        {
            char Label[32];
            snprintf(Label, sizeof(Label), "%d", Frame);
            DrawList->AddText(ImVec2(X + 4.0f, RulerTop + 4.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled), Label);
        }
    }

    const float CurrentX = std::clamp(TimeToX(State.CurrentTime), TimelineMinX, TimelineMaxX);
    DrawList->AddLine(ImVec2(CurrentX, RulerTop), ImVec2(CurrentX, CanvasEnd.y - TimelineVerticalPadding), PlayheadColor, 2.0f);
    DrawList->AddTriangleFilled(
        ImVec2(CurrentX, RulerTop + 1.0f),
        ImVec2(CurrentX - 6.0f, RulerTop + 11.0f),
        ImVec2(CurrentX + 6.0f, RulerTop + 11.0f),
        PlayheadColor);

    DrawList->PopClipRect();

    const bool bHovered = ImGui::IsItemHovered();
    const bool bActive = ImGui::IsItemActive();
    const ImVec2 MousePos = ImGui::GetIO().MousePos;

    if (bHovered && std::fabs(ImGui::GetIO().MouseWheel) > 0.0f)
    {
        const float AnchorTime = State.ClampTime(XToTime(std::clamp(MousePos.x, TimelineMinX, TimelineMaxX)));
        const float ZoomFactor = ImGui::GetIO().MouseWheel > 0.0f ? 0.85f : (1.0f / 0.85f);
        State.ZoomVisibleRange(AnchorTime, ZoomFactor);
    }

    if (bHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
    {
        const float DeltaTime = -(ImGui::GetIO().MouseDelta.x / TimelineWidth) * State.GetVisibleRange();
        State.PanVisibleRange(DeltaTime);
    }

    if (PreviewController && bActive && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        const float NewTime = State.ClampOrSnapTime(XToTime(std::clamp(MousePos.x, TimelineMinX, TimelineMaxX)));
        PreviewController->SetCurrentTime(NewTime);
        State.SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }

    DrawList->AddText(
        ImVec2(CanvasPos.x + 8.0f, CanvasEnd.y - 20.0f),
        ImGui::GetColorU32(ImGuiCol_TextDisabled),
        bHovered ? "Wheel: Zoom  Middle-drag: Pan  Left-drag: Scrub" : "Timeline");

    DrawList->AddText(
        ImVec2(CanvasEnd.x - 118.0f, CanvasEnd.y - 20.0f),
        AccentColor,
        State.bSnapToFrames ? "Snap: Frames" : "Snap: Off");

    ImGui::EndChild();
}
