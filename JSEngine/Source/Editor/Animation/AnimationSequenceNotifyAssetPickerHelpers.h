#pragma once

#include "Editor/Animation/AnimationSequenceNotifyPayloadSchema.h"

struct FNotifyAssetPickerEntry
{
    FString DisplayName;
    FString StoredValue;
};

TArray<FNotifyAssetPickerEntry> BuildNotifyAssetPickerEntries(EAnimNotifyPayloadAssetKind AssetKind);
FString ResolveNotifyAssetPickerDisplayName(
    EAnimNotifyPayloadAssetKind AssetKind,
    const FString& StoredValue,
    const TArray<FNotifyAssetPickerEntry>& Entries);
const char* GetNotifyAssetPickerEmptyMessage(EAnimNotifyPayloadAssetKind AssetKind);
const char* GetNotifyAssetPickerManualFallbackMessage(EAnimNotifyPayloadAssetKind AssetKind);
