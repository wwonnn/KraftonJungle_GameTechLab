#pragma once

#include "Animation/AnimData/FrameRate.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Object/Object.h"

#include <algorithm>
#include <cmath>

namespace AnimationSequenceViewer
{
    inline bool IsLiveObject(const UObject* Object)
    {
        return Object != nullptr && UObjectManager::Get().ContainsObject(Object);
    }

    inline const UAnimDataModel* GetValidAnimDataModel(const UAnimSequence* Sequence)
    {
        if (!IsLiveObject(Sequence))
        {
            return nullptr;
        }

        const UAnimDataModel* DataModel = Sequence->DataModel;
        return IsLiveObject(DataModel) ? DataModel : nullptr;
    }

    inline UAnimDataModel* GetValidAnimDataModel(UAnimSequence* Sequence)
    {
        return const_cast<UAnimDataModel*>(
            GetValidAnimDataModel(static_cast<const UAnimSequence*>(Sequence)));
    }

    inline FFrameRate GetSequenceFrameRate(const UAnimSequence* Sequence)
    {
        const UAnimDataModel* DataModel = GetValidAnimDataModel(Sequence);
        return DataModel ? DataModel->FrameRate : FFrameRate();
    }

    inline int32 GetSequenceFrameCount(const UAnimSequence* Sequence)
    {
        const UAnimDataModel* DataModel = GetValidAnimDataModel(Sequence);
        if (!DataModel)
        {
            return 0;
        }

        return std::max(DataModel->NumberOfFrames, DataModel->NumberOfKeys);
    }

    inline float GetSequenceLengthSeconds(const UAnimSequence* Sequence)
    {
        const UAnimDataModel* DataModel = GetValidAnimDataModel(Sequence);
        if (!DataModel)
        {
            return 0.0f;
        }

        const float FrameRate = static_cast<float>(DataModel->FrameRate.AsDecimal());
        if (FrameRate <= 0.0f || DataModel->NumberOfKeys <= 1)
        {
            return 0.0f;
        }

        return static_cast<float>(DataModel->NumberOfKeys - 1) / FrameRate;
    }

    inline float FrameIndexToTime(int32 FrameIndex, const FFrameRate& FrameRate)
    {
        const int32 SafeFrameIndex = std::max(FrameIndex, 0);
        const double FPS = FrameRate.AsDecimal();
        if (FPS <= 0.0)
        {
            return static_cast<float>(SafeFrameIndex);
        }

        return static_cast<float>(static_cast<double>(SafeFrameIndex) / FPS);
    }

    inline int32 TimeToFrameIndex(float Time, const FFrameRate& FrameRate, int32 FrameCount)
    {
        if (FrameCount <= 0)
        {
            return 0;
        }

        const double FPS = FrameRate.AsDecimal();
        if (FPS <= 0.0)
        {
            return std::clamp(
                static_cast<int32>(std::lround(Time)),
                0,
                std::max(0, FrameCount - 1));
        }

        const int32 FrameIndex = static_cast<int32>(std::lround(static_cast<double>(Time) * FPS));
        return std::clamp(FrameIndex, 0, std::max(0, FrameCount - 1));
    }
}
