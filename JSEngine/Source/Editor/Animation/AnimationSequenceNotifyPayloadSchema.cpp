#include "Editor/Animation/AnimationSequenceNotifyPayloadSchema.h"

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
                "Sound=Footstep_Stone;Socket=foot_l;Volume=0.9;Spatialized=true",
                {
                    { "Sound", "Sound", EAnimNotifyPayloadFieldType::String, true, FString(), "Required audio event or asset key." },
                    { "Socket", "Socket", EAnimNotifyPayloadFieldType::Name, false, FString(), "Optional playback socket on the preview mesh." },
                    { "Volume", "Volume", EAnimNotifyPayloadFieldType::Float, false, "1.0", "Optional volume multiplier." },
                    { "Spatialized", "Spatialized", EAnimNotifyPayloadFieldType::Bool, false, "false", "Play in 3D using the socket or mesh location." }
                }
            },
            {
                "UAnimNotifyState_PlayLoopingSFX",
                "Audio",
                "Sound=SwordHum;Socket=weapon_tip;Volume=1.0;Spatialized=true",
                {
                    { "Sound", "Sound", EAnimNotifyPayloadFieldType::String, true, FString(), "Required looping audio event or asset key." },
                    { "Socket", "Socket", EAnimNotifyPayloadFieldType::Name, false, FString(), "Optional playback socket on the preview mesh." },
                    { "Volume", "Volume", EAnimNotifyPayloadFieldType::Float, false, "1.0", "Optional volume multiplier." },
                    { "Spatialized", "Spatialized", EAnimNotifyPayloadFieldType::Bool, false, "false", "Track the socket or mesh location while the state is active." }
                }
            },
            {
                "UAnimNotifyState_AttackWindow",
                "Collision Window",
                "Component=WeaponHitbox;AttackId=Light1",
                {
                    { "Component", "Component", EAnimNotifyPayloadFieldType::String, true, FString(), "Primitive component name to toggle overlap events on." },
                    { "AttackId", "AttackId", EAnimNotifyPayloadFieldType::String, false, FString(), "Optional attack tag suffix applied as AttackId:<value>." }
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
