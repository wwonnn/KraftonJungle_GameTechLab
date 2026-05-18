#pragma once

#include "Core/CoreMinimal.h"
#include "Core/Guid.h"
#include "Math/Color.h"
#include "Object/FName.h"

struct FAnimNotifyEvent
{
    FGuid StableId;
    FName Name;
    float Time = 0.0f;
    float Duration = 0.0f;
    FColor Color = FColor(0.9490196f, 0.61960787f, 0.23921569f, 1.0f);

    void EnsureStableId()
    {
        if (!StableId.IsValid())
        {
            StableId = FGuid::NewGuid();
        }
    }
};

struct FAnimNotifyTrack
{
    FName TrackName;
    TArray<FAnimNotifyEvent> Events;
};
