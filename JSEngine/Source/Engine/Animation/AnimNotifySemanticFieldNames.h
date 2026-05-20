#pragma once

#include "Core/CoreMinimal.h"

// Semantic field ids are the shared identity layer above raw payload strings.
// Runtime, validation, schema, and editor helpers all converge here when they
// need canonical keys, legacy aliases, or lookup-key sets for one logical field.
enum class EAnimNotifySemanticFieldId : uint8
{
    None = 0,
    SoundCue,
    SocketName,
    VolumeMultiplier,
    Spatialized,
    ComponentName,
    AttackId,
    EventName,
    EndEventName,
    PayloadValue,
    Effect,
    Decal,
    Attached,
    Shake,
    Scale,
    TraceDistance,
    Size,
    Lifetime,
    AlignToHitNormal,
};

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

    EAnimNotifySemanticFieldId GetSemanticFieldId(const FString& CanonicalKey);
    const FString& GetCanonicalKey(EAnimNotifySemanticFieldId SemanticId);
    const TArray<FString>& GetLegacyAliases(EAnimNotifySemanticFieldId SemanticId);
    const TArray<FString>& GetLookupKeys(EAnimNotifySemanticFieldId SemanticId);

    const TArray<FString>& GetLegacyAliases(const FString& CanonicalKey);
    const TArray<FString>& GetLookupKeys(const FString& CanonicalKey);
}
