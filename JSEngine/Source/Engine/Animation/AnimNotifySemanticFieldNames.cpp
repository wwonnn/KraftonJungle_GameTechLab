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

        return EmptyKeys();
    }
}
