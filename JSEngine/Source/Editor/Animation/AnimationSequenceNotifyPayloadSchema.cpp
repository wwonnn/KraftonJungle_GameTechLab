#include "Editor/Animation/AnimationSequenceNotifyPayloadSchema.h"

#include "Animation/AnimNotifySemanticFieldNames.h"

#include <algorithm>

namespace
{
    const TArray<FAnimNotifyPayloadSchema>& GetAnimNotifyPayloadSchemas()
    {
        static const TArray<FAnimNotifyPayloadSchema> Schemas =
        {
            {
                "UAnimNotify_PlaySFX",
                "Audio",
                "SoundCue=Footstep_Stone;SocketName=foot_l;VolumeMultiplier=0.9;Spatialized=true",
                {
                    { AnimNotifySemanticFieldNames::SoundCueKey(), EAnimNotifySemanticFieldId::SoundCue, AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::SoundCueKey()), "Sound Cue", EAnimNotifyPayloadFieldType::String, EAnimNotifyPayloadFieldEditorHint::Default, true, FString(), "Required audio event or asset key." },
                    { AnimNotifySemanticFieldNames::SocketNameKey(), EAnimNotifySemanticFieldId::SocketName, AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::SocketNameKey()), "Socket Name", EAnimNotifyPayloadFieldType::Name, EAnimNotifyPayloadFieldEditorHint::SocketPicker, false, FString(), "Optional playback socket on the preview mesh." },
                    { AnimNotifySemanticFieldNames::VolumeMultiplierKey(), EAnimNotifySemanticFieldId::VolumeMultiplier, AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::VolumeMultiplierKey()), "Volume Multiplier", EAnimNotifyPayloadFieldType::Float, EAnimNotifyPayloadFieldEditorHint::Default, false, "1.0", "Optional volume multiplier." },
                    { AnimNotifySemanticFieldNames::SpatializedKey(), EAnimNotifySemanticFieldId::Spatialized, AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::SpatializedKey()), "Spatialized", EAnimNotifyPayloadFieldType::Bool, EAnimNotifyPayloadFieldEditorHint::Default, false, "false", "Play in 3D using the socket or mesh location." }
                }
            },
            {
                "UAnimNotifyState_PlayLoopingSFX",
                "Audio",
                "SoundCue=SwordHum;SocketName=weapon_tip;VolumeMultiplier=1.0;Spatialized=true",
                {
                    { AnimNotifySemanticFieldNames::SoundCueKey(), EAnimNotifySemanticFieldId::SoundCue, AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::SoundCueKey()), "Sound Cue", EAnimNotifyPayloadFieldType::String, EAnimNotifyPayloadFieldEditorHint::Default, true, FString(), "Required looping audio event or asset key." },
                    { AnimNotifySemanticFieldNames::SocketNameKey(), EAnimNotifySemanticFieldId::SocketName, AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::SocketNameKey()), "Socket Name", EAnimNotifyPayloadFieldType::Name, EAnimNotifyPayloadFieldEditorHint::SocketPicker, false, FString(), "Optional playback socket on the preview mesh." },
                    { AnimNotifySemanticFieldNames::VolumeMultiplierKey(), EAnimNotifySemanticFieldId::VolumeMultiplier, AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::VolumeMultiplierKey()), "Volume Multiplier", EAnimNotifyPayloadFieldType::Float, EAnimNotifyPayloadFieldEditorHint::Default, false, "1.0", "Optional volume multiplier." },
                    { AnimNotifySemanticFieldNames::SpatializedKey(), EAnimNotifySemanticFieldId::Spatialized, AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::SpatializedKey()), "Spatialized", EAnimNotifyPayloadFieldType::Bool, EAnimNotifyPayloadFieldEditorHint::Default, false, "false", "Track the socket or mesh location while the state is active." }
                }
            },
            {
                "UAnimNotifyState_AttackWindow",
                "Collision Window",
                "ComponentName=WeaponHitbox;AttackId=Light1",
                {
                    { AnimNotifySemanticFieldNames::ComponentNameKey(), EAnimNotifySemanticFieldId::ComponentName, AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::ComponentNameKey()), "Component Name", EAnimNotifyPayloadFieldType::String, EAnimNotifyPayloadFieldEditorHint::ComponentPicker, true, FString(), "Primitive component name to toggle overlap events on." },
                    { AnimNotifySemanticFieldNames::AttackIdKey(), EAnimNotifySemanticFieldId::AttackId, AnimNotifySemanticFieldNames::GetLegacyAliases(AnimNotifySemanticFieldNames::AttackIdKey()), "AttackId", EAnimNotifyPayloadFieldType::String, EAnimNotifyPayloadFieldEditorHint::Default, false, FString(), "Optional attack tag suffix applied as AttackId:<value>." }
                }
            }
        };
        return Schemas;
    }
}

const FAnimNotifyPayloadSchema* FindAnimNotifyPayloadSchema(const FString& NotifyClassName)
{
    const TArray<FAnimNotifyPayloadSchema>& Schemas = GetAnimNotifyPayloadSchemas();
    const auto Iterator = std::find_if(
        Schemas.begin(),
        Schemas.end(),
        [&NotifyClassName](const FAnimNotifyPayloadSchema& Schema)
        {
            return Schema.NotifyClassName == NotifyClassName;
        });
    return Iterator != Schemas.end() ? &(*Iterator) : nullptr;
}

bool HasAnimNotifyPayloadSchema(const FString& NotifyClassName)
{
    return FindAnimNotifyPayloadSchema(NotifyClassName) != nullptr;
}

const char* GetAnimNotifyPayloadExample(const FString& NotifyClassName)
{
    const FAnimNotifyPayloadSchema* Schema = FindAnimNotifyPayloadSchema(NotifyClassName);
    return (Schema && !Schema->PayloadExample.empty()) ? Schema->PayloadExample.c_str() : nullptr;
}

TArray<FString> GetAnimNotifyPayloadFieldLookupKeys(const FAnimNotifyPayloadFieldDefinition& Field)
{
    TArray<FString> Keys;
    if (!Field.Key.empty())
    {
        Keys.push_back(Field.Key);
    }

    for (const FString& LegacyKey : Field.LegacyKeys)
    {
        if (!LegacyKey.empty())
        {
            Keys.push_back(LegacyKey);
        }
    }

    return Keys;
}

const FAnimNotifyPayloadFieldDefinition* FindAnimNotifyPayloadFieldBySemanticId(
    const FAnimNotifyPayloadSchema& Schema,
    EAnimNotifySemanticFieldId SemanticId)
{
    const auto Iterator = std::find_if(
        Schema.Fields.begin(),
        Schema.Fields.end(),
        [SemanticId](const FAnimNotifyPayloadFieldDefinition& Field)
        {
            return Field.SemanticId == SemanticId;
        });
    return Iterator != Schema.Fields.end() ? &(*Iterator) : nullptr;
}
