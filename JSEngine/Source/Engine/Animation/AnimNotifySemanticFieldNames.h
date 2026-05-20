#pragma once

#include "Core/CoreMinimal.h"

namespace AnimNotifySemanticFieldNames
{
    const FString& SoundCueKey();
    const FString& LegacySoundKey();

    const FString& SocketNameKey();
    const FString& LegacySocketKey();

    const FString& VolumeMultiplierKey();
    const FString& LegacyVolumeKey();

    const FString& SpatializedKey();

    const FString& ComponentNameKey();
    const FString& LegacyComponentKey();

    const FString& AttackIdKey();
    const FString& EventNameKey();
    const FString& EndEventNameKey();
    const FString& PayloadKey();
    const FString& EffectKey();
    const FString& DecalKey();
    const FString& AttachedKey();
    const FString& ShakeKey();
    const FString& ScaleKey();
    const FString& TraceDistanceKey();
    const FString& SizeKey();
    const FString& LifetimeKey();
    const FString& AlignToHitNormalKey();

    const TArray<FString>& GetLegacyAliases(const FString& CanonicalKey);
    const TArray<FString>& GetLookupKeys(const FString& CanonicalKey);
}
