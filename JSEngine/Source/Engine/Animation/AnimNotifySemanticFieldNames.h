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

    const TArray<FString>& GetLegacyAliases(const FString& CanonicalKey);
    const TArray<FString>& GetLookupKeys(const FString& CanonicalKey);
}
