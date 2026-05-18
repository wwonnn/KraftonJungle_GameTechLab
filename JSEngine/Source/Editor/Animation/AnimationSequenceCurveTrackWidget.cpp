#include "Editor/Animation/AnimationSequenceCurveTrackWidget.h"

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Editor/Animation/AnimationSequenceEditorState.h"
#include "Editor/Animation/AnimationSequenceSequencerLayout.h"
#include "Editor/Animation/AnimationSequenceTimelineGeometry.h"
#include "Editor/Animation/AnimationSequenceViewerUtils.h"
#include "Engine/Asset/CurveFloatAsset.h"

#include "ImGui/imgui.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <string>

namespace
{
    struct FCurveRange
    {
        float MinValue = 0.0f;
        float MaxValue = 1.0f;
    };

    FCurveRange ComputeCurveRange(const FFloatCurve& Curve, float StartTime, float EndTime)
    {
        FCurveRange Range;
        Range.MinValue = FLT_MAX;
        Range.MaxValue = -FLT_MAX;

        constexpr int32 SampleCount = 64;
        const float SafeStart = std::min(StartTime, EndTime);
        const float SafeEnd = std::max(StartTime, EndTime);
        for (int32 Index = 0; Index < SampleCount; ++Index)
        {
            const float Alpha = SampleCount > 1 ? static_cast<float>(Index) / static_cast<float>(SampleCount - 1) : 0.0f;
            const float Time = SafeStart + (SafeEnd - SafeStart) * Alpha;
            const float Value = Curve.Evaluate(Time);
            Range.MinValue = std::min(Range.MinValue, Value);
            Range.MaxValue = std::max(Range.MaxValue, Value);
        }

        if (Range.MinValue == FLT_MAX || Range.MaxValue == -FLT_MAX)
        {
            Range.MinValue = 0.0f;
            Range.MaxValue = 1.0f;
        }
        else if (std::abs(Range.MaxValue - Range.MinValue) < 0.0001f)
        {
            Range.MinValue -= 0.5f;
            Range.MaxValue += 0.5f;
        }

        return Range;
    }

    ImU32 GetCurveColor(int32 CurveIndex, bool bSelected)
    {
        static constexpr ImU32 Palette[] =
        {
            IM_COL32(123, 176, 255, 255),
            IM_COL32(163, 219, 103, 255),
            IM_COL32(243, 213, 92, 255),
            IM_COL32(239, 134, 226, 255),
            IM_COL32(144, 195, 255, 255),
            IM_COL32(255, 178, 107, 255),
        };

        const ImU32 Base = Palette[CurveIndex % (sizeof(Palette) / sizeof(Palette[0]))];
        if (!bSelected)
        {
            return Base;
        }

        return IM_COL32(255, 255, 255, 255);
    }

    float SampleCurveY(
        const FFloatCurve& Curve,
        const FAnimationSequenceEditorState& State,
        const FCurveRange& Range,
        float Time,
        float RowTop,
        float RowBottom)
    {
        const float Value = Curve.Evaluate(Time);
        const float RangeSize = std::max(Range.MaxValue - Range.MinValue, 0.0001f);
        const float Alpha = (Value - Range.MinValue) / RangeSize;
        return std::clamp(RowBottom - Alpha * (RowBottom - RowTop), RowTop, RowBottom);
    }
}

void FAnimationSequenceCurveTrackWidget::RenderRows(
    const UAnimSequence* Sequence,
    FAnimationSequenceEditorState& State,
    const FAnimationSequenceTimelineGeometry& Geometry,
    float SectionTop) const
{
    State.HoveredCurveIndex = -1;
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    const UAnimDataModel* DataModel = AnimationSequenceViewer::GetValidAnimDataModel(Sequence);
    const int32 CurveCount = DataModel ? static_cast<int32>(DataModel->CurveData.FloatCurves.size()) : 0;

    const ImVec2 HeaderMin(Geometry.CanvasPos.x + 4.0f, SectionTop);
    const ImVec2 HeaderMax(Geometry.CanvasEnd.x - 4.0f, SectionTop + FAnimationSequenceSequencerLayout::SectionHeaderHeight);
    DrawList->AddRectFilled(HeaderMin, HeaderMax, IM_COL32(29, 34, 41, 255), 4.0f);
    DrawList->AddRect(HeaderMin, HeaderMax, IM_COL32(255, 255, 255, 18), 4.0f);
    DrawList->AddText(ImVec2(HeaderMin.x + 8.0f, HeaderMin.y + 4.0f), IM_COL32(208, 214, 226, 255), "Curves");

    if (!State.bCurvesExpanded || !DataModel || CurveCount <= 0)
    {
        return;
    }

    const float RowsTop = SectionTop + FAnimationSequenceSequencerLayout::SectionHeaderHeight + 4.0f;
    for (int32 CurveIndex = 0; CurveIndex < CurveCount; ++CurveIndex)
    {
        const FFloatCurve& Curve = DataModel->CurveData.FloatCurves[CurveIndex];
        const float RowTop =
            RowsTop +
            CurveIndex *
            (FAnimationSequenceSequencerLayout::CurveTrackRowHeight + FAnimationSequenceSequencerLayout::CurveTrackRowSpacing);
        const float RowBottom = RowTop + FAnimationSequenceSequencerLayout::CurveTrackRowHeight;
        const bool bSelected = State.SelectedCurveIndex == CurveIndex;
        const bool bHovered = State.HoveredCurveIndex == CurveIndex;

        const ImVec2 RowMin(Geometry.TimelineMinX, RowTop);
        const ImVec2 RowMax(Geometry.TimelineMaxX, RowBottom);
        DrawList->AddRectFilled(
            RowMin,
            RowMax,
            bSelected ? IM_COL32(38, 48, 64, 255) : (bHovered ? IM_COL32(28, 33, 40, 255) : IM_COL32(22, 25, 31, 220)),
            4.0f);
        DrawList->AddRect(RowMin, RowMax, IM_COL32(255, 255, 255, 18), 4.0f);

        ImGui::SetCursorScreenPos(RowMin);
        ImGui::PushID(CurveIndex + 7000);
        ImGui::InvisibleButton(
            "##CurveRow",
            ImVec2(RowMax.x - RowMin.x, RowMax.y - RowMin.y));
        if (ImGui::IsItemHovered())
        {
            State.HoveredCurveIndex = CurveIndex;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            State.SelectedCurveIndex = CurveIndex;
        }
        ImGui::PopID();

        const FCurveRange ValueRange = ComputeCurveRange(Curve, State.VisibleTimeStart, State.VisibleTimeEnd);
        constexpr int32 SampleCount = 96;
        ImVec2 PreviousPoint = {};
        bool bHasPreviousPoint = false;

        for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
        {
            const float Alpha = SampleCount > 1 ? static_cast<float>(SampleIndex) / static_cast<float>(SampleCount - 1) : 0.0f;
            const float Time = State.VisibleTimeStart + State.GetVisibleRange() * Alpha;
            const float X = Geometry.TimeToX(State, Time);
            const float Y = SampleCurveY(Curve, State, ValueRange, Time, RowTop + 6.0f, RowBottom - 6.0f);
            const ImVec2 Point(X, Y);
            if (bHasPreviousPoint)
            {
                DrawList->AddLine(PreviousPoint, Point, GetCurveColor(CurveIndex, bSelected), bSelected ? 2.4f : 1.8f);
            }

            PreviousPoint = Point;
            bHasPreviousPoint = true;
        }

        DrawList->AddText(
            ImVec2(Geometry.TimelineMaxX - 112.0f, RowTop + 4.0f),
            IM_COL32(140, 149, 163, 255),
            (std::to_string(static_cast<int32>(Curve.Keys.size())) + " keys").c_str());
    }
}
