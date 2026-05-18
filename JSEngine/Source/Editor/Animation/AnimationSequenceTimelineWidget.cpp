#include "Editor/Animation/AnimationSequenceTimelineWidget.h"

#include "Editor/Animation/AnimationSequenceEditorState.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"
#include "Editor/Animation/AnimationSequenceTimelineGeometry.h"

#include "ImGui/imgui.h"

#include <algorithm>
#include <cstdio>
#include <cmath>

namespace
{
    constexpr float TimelineHeight = 92.0f;
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
    const FAnimationSequenceTimelineGeometry Geometry =
        FAnimationSequenceTimelineGeometry::BuildTimelineGeometry(CanvasPos, CanvasSize, RulerHeight);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImU32 BorderColor = ImGui::GetColorU32(ImGuiCol_Border);
    const ImU32 PanelColor = ImGui::GetColorU32(ImGuiCol_ChildBg);
    const ImU32 RulerColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
    const ImU32 GridColor = WithAlpha(ImGui::GetColorU32(ImGuiCol_TextDisabled), 0.34f);
    const ImU32 MinorGridColor = WithAlpha(ImGui::GetColorU32(ImGuiCol_TextDisabled), 0.16f);
    const ImU32 PlayheadColor = IM_COL32(255, 106, 72, 255);
    const ImU32 AccentColor = IM_COL32(255, 170, 72, 255);

    DrawList->AddRectFilled(CanvasPos, Geometry.CanvasEnd, PanelColor);
    DrawList->AddRectFilled(
        ImVec2(Geometry.TimelineMinX, Geometry.RulerTop),
        ImVec2(Geometry.TimelineMaxX, Geometry.CanvasEnd.y - FAnimationSequenceTimelineGeometry::VerticalPadding),
        RulerColor,
        4.0f);
    DrawList->AddRect(CanvasPos, Geometry.CanvasEnd, BorderColor);
    DrawList->AddLine(
        ImVec2(Geometry.TimelineMinX, Geometry.TrackTop),
        ImVec2(Geometry.TimelineMaxX, Geometry.TrackTop),
        BorderColor);

    if (!State.HasTimelineData())
    {
        DrawList->AddText(
            ImVec2(Geometry.TimelineMinX, Geometry.TrackTop + 10.0f),
            ImGui::GetColorU32(ImGuiCol_TextDisabled),
            "Timeline data is unavailable.");
        ImGui::EndChild();
        return;
    }

    const int32 MajorStep = State.ChooseMajorFrameStep(Geometry.TimelineWidth);
    const int32 MinorStep = State.ChooseMinorFrameStep(Geometry.TimelineWidth);
    const int32 EndFrame = Geometry.GetEndFrame(State);
    const int32 FirstMinorFrame = Geometry.GetFirstMinorFrame(State, MinorStep);

    DrawList->PushClipRect(
        ImVec2(Geometry.TimelineMinX, Geometry.RulerTop),
        ImVec2(Geometry.TimelineMaxX, Geometry.CanvasEnd.y - FAnimationSequenceTimelineGeometry::VerticalPadding),
        true);

    for (int32 Frame = FirstMinorFrame; Frame <= EndFrame + MinorStep; Frame += std::max(MinorStep, 1))
    {
        const float Time = State.FrameToTime(Frame);
        const float X = Geometry.TimeToX(State, Time);
        if (X < Geometry.TimelineMinX - 1.0f || X > Geometry.TimelineMaxX + 1.0f)
        {
            continue;
        }

        const bool bMajorTick = MajorStep > 0 && (Frame % MajorStep) == 0;
        DrawList->AddLine(
            ImVec2(X, bMajorTick ? Geometry.RulerTop : Geometry.TrackTop - 10.0f),
            ImVec2(X, Geometry.CanvasEnd.y - FAnimationSequenceTimelineGeometry::VerticalPadding),
            bMajorTick ? GridColor : MinorGridColor);

        if (bMajorTick)
        {
            char Label[32];
            snprintf(Label, sizeof(Label), "%d", Frame);
            DrawList->AddText(
                ImVec2(X + 4.0f, Geometry.RulerTop + 4.0f),
                ImGui::GetColorU32(ImGuiCol_TextDisabled),
                Label);
        }
    }

    const float CurrentX = Geometry.GetClampedPlayheadX(State);
    DrawList->AddLine(
        ImVec2(CurrentX, Geometry.RulerTop),
        ImVec2(CurrentX, Geometry.CanvasEnd.y - FAnimationSequenceTimelineGeometry::VerticalPadding),
        PlayheadColor,
        2.0f);
    DrawList->AddTriangleFilled(
        ImVec2(CurrentX, Geometry.RulerTop + 1.0f),
        ImVec2(CurrentX - 6.0f, Geometry.RulerTop + 11.0f),
        ImVec2(CurrentX + 6.0f, Geometry.RulerTop + 11.0f),
        PlayheadColor);

    DrawList->PopClipRect();

    const bool bHovered = ImGui::IsItemHovered();
    const bool bActive = ImGui::IsItemActive();
    const ImVec2 MousePos = ImGui::GetIO().MousePos;

    if (bHovered && std::fabs(ImGui::GetIO().MouseWheel) > 0.0f)
    {
        const float AnchorTime = State.ClampTime(
            Geometry.XToTime(State, std::clamp(MousePos.x, Geometry.TimelineMinX, Geometry.TimelineMaxX)));
        const float ZoomFactor = ImGui::GetIO().MouseWheel > 0.0f ? 0.85f : (1.0f / 0.85f);
        State.ZoomVisibleRange(AnchorTime, ZoomFactor);
    }

    if (bHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
    {
        const float DeltaTime = -(ImGui::GetIO().MouseDelta.x / Geometry.TimelineWidth) * State.GetVisibleRange();
        State.PanVisibleRange(DeltaTime);
    }

    if (PreviewController && bActive && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        const float NewTime = State.ClampOrSnapTime(
            Geometry.XToTime(
                State,
                std::clamp(MousePos.x, Geometry.TimelineMinX, Geometry.TimelineMaxX)));
        PreviewController->SetCurrentTime(NewTime);
        State.SetCurrentTime(PreviewController->GetCurrentTime(), false);
    }

    DrawList->AddText(
        ImVec2(CanvasPos.x + 8.0f, Geometry.CanvasEnd.y - 20.0f),
        ImGui::GetColorU32(ImGuiCol_TextDisabled),
        bHovered ? "Wheel: Zoom  Middle-drag: Pan  Left-drag: Scrub" : "Timeline");

    DrawList->AddText(
        ImVec2(Geometry.CanvasEnd.x - 118.0f, Geometry.CanvasEnd.y - 20.0f),
        AccentColor,
        State.bSnapToFrames ? "Snap: Frames" : "Snap: Off");

    ImGui::EndChild();
}
