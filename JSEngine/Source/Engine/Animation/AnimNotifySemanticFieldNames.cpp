#include "Animation/AnimNotifySemanticFieldNames.h"

namespace
{
    const TArray<FString>& EmptyKeys()
    {
        static const TArray<FString> Keys;
        return Keys;
    }
}

namespace AnimNotifySemanticFieldNames
{
    const FString& SoundCueKey()
    {
        static const FString Key = "SoundCue";
        return Key;
    }

    const FString& LegacySoundKey()
    {
        static const FString Key = "Sound";
        return Key;
    }

    const FString& SocketNameKey()
    {
        static const FString Key = "SocketName";
        return Key;
    }

    const FString& LegacySocketKey()
    {
        static const FString Key = "Socket";
        return Key;
    }

    const FString& VolumeMultiplierKey()
    {
        static const FString Key = "VolumeMultiplier";
        return Key;
    }

    const FString& LegacyVolumeKey()
    {
        static const FString Key = "Volume";
        return Key;
    }

    const FString& SpatializedKey()
    {
        static const FString Key = "Spatialized";
        return Key;
    }

    const FString& ComponentNameKey()
    {
        static const FString Key = "ComponentName";
        return Key;
    }

    const FString& LegacyComponentKey()
    {
        static const FString Key = "Component";
        return Key;
    }

    const FString& AttackIdKey()
    {
        static const FString Key = "AttackId";
        return Key;
    }

    const FString& EventNameKey()
    {
        static const FString Key = "EventName";
        return Key;
    }

    const FString& EndEventNameKey()
    {
        static const FString Key = "EndEventName";
        return Key;
    }

    const FString& PayloadKey()
    {
        static const FString Key = "Payload";
        return Key;
    }

    const FString& EffectKey()
    {
        static const FString Key = "Effect";
        return Key;
    }

    const FString& DecalKey()
    {
        static const FString Key = "Decal";
        return Key;
    }

    const FString& AttachedKey()
    {
        static const FString Key = "Attached";
        return Key;
    }

    const FString& ShakeKey()
    {
        static const FString Key = "Shake";
        return Key;
    }

    const FString& ScaleKey()
    {
        static const FString Key = "Scale";
        return Key;
    }

    const FString& TraceDistanceKey()
    {
        static const FString Key = "TraceDistance";
        return Key;
    }

    const FString& SizeKey()
    {
        static const FString Key = "Size";
        return Key;
    }

    const FString& LifetimeKey()
    {
        static const FString Key = "Lifetime";
        return Key;
    }

    const FString& AlignToHitNormalKey()
    {
        static const FString Key = "AlignToHitNormal";
        return Key;
    }

    const TArray<FString>& GetLegacyAliases(const FString& CanonicalKey)
    {
        static const TArray<FString> SoundCueAliases = { LegacySoundKey() };
        static const TArray<FString> SocketNameAliases = { LegacySocketKey() };
        static const TArray<FString> VolumeMultiplierAliases = { LegacyVolumeKey() };
        static const TArray<FString> ComponentNameAliases = { LegacyComponentKey() };

        if (CanonicalKey == SoundCueKey())
        {
            return SoundCueAliases;
        }

        if (CanonicalKey == SocketNameKey())
        {
            return SocketNameAliases;
        }

        if (CanonicalKey == VolumeMultiplierKey())
        {
            return VolumeMultiplierAliases;
        }

        if (CanonicalKey == ComponentNameKey())
        {
            return ComponentNameAliases;
        }

        return EmptyKeys();
    }

    const TArray<FString>& GetLookupKeys(const FString& CanonicalKey)
    {
        static const TArray<FString> SoundCueKeys = { SoundCueKey(), LegacySoundKey() };
        static const TArray<FString> SocketNameKeys = { SocketNameKey(), LegacySocketKey() };
        static const TArray<FString> VolumeMultiplierKeys = { VolumeMultiplierKey(), LegacyVolumeKey() };
        static const TArray<FString> SpatializedKeys = { SpatializedKey() };
        static const TArray<FString> ComponentNameKeys = { ComponentNameKey(), LegacyComponentKey() };
        static const TArray<FString> AttackIdKeys = { AttackIdKey() };
        static const TArray<FString> EventNameKeys = { EventNameKey() };
        static const TArray<FString> EndEventNameKeys = { EndEventNameKey() };
        static const TArray<FString> PayloadKeys = { PayloadKey() };
        static const TArray<FString> EffectKeys = { EffectKey() };
        static const TArray<FString> DecalKeys = { DecalKey() };
        static const TArray<FString> AttachedKeys = { AttachedKey() };
        static const TArray<FString> ShakeKeys = { ShakeKey() };
        static const TArray<FString> ScaleKeys = { ScaleKey() };
        static const TArray<FString> TraceDistanceKeys = { TraceDistanceKey() };
        static const TArray<FString> SizeKeys = { SizeKey() };
        static const TArray<FString> LifetimeKeys = { LifetimeKey() };
        static const TArray<FString> AlignToHitNormalKeys = { AlignToHitNormalKey() };

        if (CanonicalKey == SoundCueKey())
        {
            return SoundCueKeys;
        }

        if (CanonicalKey == SocketNameKey())
        {
            return SocketNameKeys;
        }

        if (CanonicalKey == VolumeMultiplierKey())
        {
            return VolumeMultiplierKeys;
        }

        if (CanonicalKey == SpatializedKey())
        {
            return SpatializedKeys;
        }

        if (CanonicalKey == ComponentNameKey())
        {
            return ComponentNameKeys;
        }

        if (CanonicalKey == AttackIdKey())
        {
            return AttackIdKeys;
        }

        if (CanonicalKey == EventNameKey())
        {
            return EventNameKeys;
        }

        if (CanonicalKey == EndEventNameKey())
        {
            return EndEventNameKeys;
        }

        if (CanonicalKey == PayloadKey())
        {
            return PayloadKeys;
        }

        if (CanonicalKey == EffectKey())
        {
            return EffectKeys;
        }

        if (CanonicalKey == DecalKey())
        {
            return DecalKeys;
        }

        if (CanonicalKey == AttachedKey())
        {
            return AttachedKeys;
        }

        if (CanonicalKey == ShakeKey())
        {
            return ShakeKeys;
        }

        if (CanonicalKey == ScaleKey())
        {
            return ScaleKeys;
        }

        if (CanonicalKey == TraceDistanceKey())
        {
            return TraceDistanceKeys;
        }

        if (CanonicalKey == SizeKey())
        {
            return SizeKeys;
        }

        if (CanonicalKey == LifetimeKey())
        {
            return LifetimeKeys;
        }

        if (CanonicalKey == AlignToHitNormalKey())
        {
            return AlignToHitNormalKeys;
        }

        return EmptyKeys();
    }
}
