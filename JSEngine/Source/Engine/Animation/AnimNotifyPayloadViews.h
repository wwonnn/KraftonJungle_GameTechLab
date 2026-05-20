#pragma once

#include "Core/CoreMinimal.h"
#include "Object/FName.h"

class FAnimNotifyPayloadParser;

struct FPlaySfxPayloadView
{
    FString SoundCue;
    FName SocketName = FName::None;
    float VolumeMultiplier = 1.0f;
    bool bSpatialized = false;
};

struct FPlayVfxPayloadView
{
    FString Effect;
    FName SocketName = FName::None;
    bool bAttached = false;
    float Scale = 1.0f;
};

struct FCameraShakePayloadView
{
    FString Shake;
    float Scale = 1.0f;
};

FPlaySfxPayloadView BuildPlaySfxPayloadView(const FAnimNotifyPayloadParser& Payload);
FPlayVfxPayloadView BuildPlayVfxPayloadView(const FAnimNotifyPayloadParser& Payload);
FCameraShakePayloadView BuildCameraShakePayloadView(const FAnimNotifyPayloadParser& Payload);
