#include "Animation/AnimNotifyBuiltins.h"

#include "Animation/AnimData/AnimSequence.h"
#include "Animation/AnimNotifyPayloadParser.h"
#include "Animation/AnimNotifySemanticFieldNames.h"
#include "Component/PrimitiveComponent.h"
#include "Component/SkeletalMeshComponent.h"
#include "Core/Logging/Log.h"
#include "GameFramework/AActor.h"
#include "Object/ObjectFactory.h"
#include "Runtime/Engine.h"

namespace
{
    constexpr const char* AttackIdTagPrefix = "AttackId:";

    struct FAttackWindowComponentState
    {
        int32 RefCount = 0;
        bool bOriginalGenerateOverlapEvents = false;
    };

    struct FAttackWindowTagState
    {
        int32 RefCount = 0;
        bool bOwnedByNotifySystem = false;
    };

    TMap<UPrimitiveComponent*, FAttackWindowComponentState> GAttackWindowComponentStates;
    TMap<FString, FAttackWindowTagState> GAttackWindowTagStates;

    bool IsLiveObject(const UObject* Object)
    {
        return Object != nullptr && UObjectManager::Get().ContainsObject(Object);
    }

    bool IsLiveActor(const AActor* Actor)
    {
        return Actor != nullptr && UObjectManager::Get().ContainsObject(Actor) && !Actor->IsPendingKill();
    }

    FString GetNotifyLabel(const FAnimNotifyEvent& NotifyEvent)
    {
        return NotifyEvent.Name.IsValid() ? NotifyEvent.Name.ToString() : FString("(UnnamedNotify)");
    }

    AActor* ResolveOwnerActor(USkeletalMeshComponent* MeshComponent)
    {
        AActor* OwnerActor = MeshComponent ? MeshComponent->GetOwner() : nullptr;
        return IsLiveActor(OwnerActor) ? OwnerActor : nullptr;
    }

    FVector ResolvePlaybackLocation(USkeletalMeshComponent* MeshComponent, const FName& SocketName)
    {
        if (MeshComponent && SocketName != FName::None && MeshComponent->HasSocket(SocketName))
        {
            return MeshComponent->GetSocketTransform(SocketName).GetLocation();
        }

        return MeshComponent ? MeshComponent->GetWorldLocation() : FVector::ZeroVector;
    }

    UPrimitiveComponent* FindPrimitiveComponentByName(AActor* OwnerActor, const FString& ComponentName)
    {
        if (!IsLiveActor(OwnerActor) || ComponentName.empty())
        {
            return nullptr;
        }

        for (UActorComponent* Component : OwnerActor->GetComponents())
        {
            UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
            if (!PrimitiveComponent || !IsLiveObject(PrimitiveComponent))
            {
                continue;
            }

            if (PrimitiveComponent->GetName() == ComponentName)
            {
                return PrimitiveComponent;
            }
        }

        return nullptr;
    }

    FString BuildAttackIdTag(const FString& AttackId)
    {
        return AttackId.empty() ? FString() : FString(AttackIdTagPrefix) + AttackId;
    }

    FString BuildAttackTagStateKey(const AActor* OwnerActor, const FString& AttackTag)
    {
        return std::to_string(OwnerActor ? OwnerActor->GetUUID() : 0) + "|" + AttackTag;
    }

    bool AcquireAttackWindow(UPrimitiveComponent* PrimitiveComponent)
    {
        if (!IsLiveObject(PrimitiveComponent))
        {
            return false;
        }

        FAttackWindowComponentState& State = GAttackWindowComponentStates[PrimitiveComponent];
        if (State.RefCount == 0)
        {
            State.bOriginalGenerateOverlapEvents = PrimitiveComponent->ShouldGenerateOverlapEvents();
            PrimitiveComponent->SetGenerateOverlapEvents(true);
        }

        ++State.RefCount;
        return true;
    }

    void ReleaseAttackWindow(UPrimitiveComponent* PrimitiveComponent)
    {
        const auto Iterator = GAttackWindowComponentStates.find(PrimitiveComponent);
        if (Iterator == GAttackWindowComponentStates.end())
        {
            return;
        }

        FAttackWindowComponentState& State = Iterator->second;
        if (State.RefCount > 0)
        {
            --State.RefCount;
        }

        if (State.RefCount <= 0)
        {
            if (IsLiveObject(PrimitiveComponent))
            {
                PrimitiveComponent->SetGenerateOverlapEvents(State.bOriginalGenerateOverlapEvents);
            }
            GAttackWindowComponentStates.erase(Iterator);
        }
    }

    FString AcquireAttackTagStateKey(AActor* OwnerActor, const FString& AttackTag)
    {
        if (!IsLiveActor(OwnerActor) || AttackTag.empty())
        {
            return FString();
        }

        const FString StateKey = BuildAttackTagStateKey(OwnerActor, AttackTag);
        FAttackWindowTagState& State = GAttackWindowTagStates[StateKey];
        if (State.RefCount == 0)
        {
            State.bOwnedByNotifySystem = !OwnerActor->HasTag(AttackTag);
            if (State.bOwnedByNotifySystem)
            {
                OwnerActor->AddTag(AttackTag);
            }
        }

        ++State.RefCount;
        return StateKey;
    }

    void ReleaseAttackTag(AActor* OwnerActor, const FString& StateKey, const FString& AttackTag)
    {
        if (StateKey.empty() || AttackTag.empty())
        {
            return;
        }

        const auto Iterator = GAttackWindowTagStates.find(StateKey);
        if (Iterator == GAttackWindowTagStates.end())
        {
            return;
        }

        FAttackWindowTagState& State = Iterator->second;
        if (State.RefCount > 0)
        {
            --State.RefCount;
        }

        if (State.RefCount <= 0)
        {
            if (State.bOwnedByNotifySystem && IsLiveActor(OwnerActor))
            {
                OwnerActor->RemoveTag(AttackTag);
            }
            GAttackWindowTagStates.erase(Iterator);
        }
    }
}

DEFINE_CLASS(UAnimNotify_PlaySFX, UAnimNotify)
REGISTER_FACTORY(UAnimNotify_PlaySFX)

DEFINE_CLASS(UAnimNotifyState_PlayLoopingSFX, UAnimNotifyState)
REGISTER_FACTORY(UAnimNotifyState_PlayLoopingSFX)

DEFINE_CLASS(UAnimNotifyState_AttackWindow, UAnimNotifyState)
REGISTER_FACTORY(UAnimNotifyState_AttackWindow)

void UAnimNotify_PlaySFX::Notify(
    USkeletalMeshComponent* MeshComponent,
    UAnimSequence* Animation,
    const FAnimNotifyEvent& NotifyEvent)
{
    (void)Animation;

    if (!GEngine)
    {
        return;
    }

    const FAnimNotifyPayloadParser Payload(NotifyEvent.Payload);
    const FString SoundKeyOrPath = Payload.GetStringAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::SoundCueKey()));
    if (SoundKeyOrPath.empty())
    {
        UE_LOG_WARNING("[AnimNotifyBuiltins] PlaySFX notify missing SoundCue payload | Notify=%s", GetNotifyLabel(NotifyEvent).c_str());
        return;
    }

    const FName SocketName = Payload.GetNameAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::SocketNameKey()));
    const float Volume = std::max(0.0f, Payload.GetFloatAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::VolumeMultiplierKey()), 1.0f));
    const bool bSpatialized = Payload.GetBoolAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::SpatializedKey()), false);

    if (bSpatialized)
    {
        GEngine->GetAudioSystem().PlaySoundCue(
            SoundKeyOrPath,
            false,
            true,
            ResolvePlaybackLocation(MeshComponent, SocketName),
            Volume);
        return;
    }

    GEngine->GetAudioSystem().PlaySFX(SoundKeyOrPath, Volume);
}

UAnimNotifyState_PlayLoopingSFX::~UAnimNotifyState_PlayLoopingSFX()
{
    Cleanup();
}

void UAnimNotifyState_PlayLoopingSFX::NotifyBegin(
    USkeletalMeshComponent* MeshComponent,
    UAnimSequence* Animation,
    const FAnimNotifyEvent& NotifyEvent)
{
    (void)Animation;
    Cleanup();

    if (!GEngine)
    {
        return;
    }

    const FAnimNotifyPayloadParser Payload(NotifyEvent.Payload);
    const FString SoundKeyOrPath = Payload.GetStringAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::SoundCueKey()));
    if (SoundKeyOrPath.empty())
    {
        UE_LOG_WARNING("[AnimNotifyBuiltins] LoopingSFX notify missing SoundCue payload | Notify=%s", GetNotifyLabel(NotifyEvent).c_str());
        return;
    }

    const FName SocketName = Payload.GetNameAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::SocketNameKey()));
    const float Volume = std::max(0.0f, Payload.GetFloatAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::VolumeMultiplierKey()), 1.0f));
    bSpatialized = Payload.GetBoolAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::SpatializedKey()), false);

    ActiveHandle = GEngine->GetAudioSystem().PlaySoundCue(
        SoundKeyOrPath,
        true,
        bSpatialized,
        ResolvePlaybackLocation(MeshComponent, SocketName),
        Volume);
}

void UAnimNotifyState_PlayLoopingSFX::NotifyTick(
    USkeletalMeshComponent* MeshComponent,
    UAnimSequence* Animation,
    const FAnimNotifyEvent& NotifyEvent,
    float DeltaTime)
{
    (void)Animation;
    (void)NotifyEvent;
    (void)DeltaTime;

    if (!bSpatialized || !GEngine || ActiveHandle == 0)
    {
        return;
    }

    const FAnimNotifyPayloadParser Payload(NotifyEvent.Payload);
    const FName SocketName = Payload.GetNameAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::SocketNameKey()));
    GEngine->GetAudioSystem().SetSoundPosition(ActiveHandle, ResolvePlaybackLocation(MeshComponent, SocketName));
}

void UAnimNotifyState_PlayLoopingSFX::NotifyEnd(
    USkeletalMeshComponent* MeshComponent,
    UAnimSequence* Animation,
    const FAnimNotifyEvent& NotifyEvent)
{
    (void)MeshComponent;
    (void)Animation;
    (void)NotifyEvent;
    Cleanup();
}

void UAnimNotifyState_PlayLoopingSFX::Cleanup()
{
    if (GEngine && ActiveHandle != 0)
    {
        GEngine->GetAudioSystem().StopSound(ActiveHandle);
    }

    ActiveHandle = 0;
    bSpatialized = false;
}

UAnimNotifyState_AttackWindow::~UAnimNotifyState_AttackWindow()
{
    Cleanup();
}

void UAnimNotifyState_AttackWindow::NotifyBegin(
    USkeletalMeshComponent* MeshComponent,
    UAnimSequence* Animation,
    const FAnimNotifyEvent& NotifyEvent)
{
    (void)Animation;
    Cleanup();

    CachedOwnerActor = ResolveOwnerActor(MeshComponent);
    if (!CachedOwnerActor)
    {
        UE_LOG_WARNING("[AnimNotifyBuiltins] AttackWindow notify missing owner actor | Notify=%s", GetNotifyLabel(NotifyEvent).c_str());
        return;
    }

    const FAnimNotifyPayloadParser Payload(NotifyEvent.Payload);
    const FString ComponentName = Payload.GetStringAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::ComponentNameKey()));
    if (ComponentName.empty())
    {
        UE_LOG_WARNING("[AnimNotifyBuiltins] AttackWindow notify missing ComponentName payload | Notify=%s", GetNotifyLabel(NotifyEvent).c_str());
        CachedOwnerActor = nullptr;
        return;
    }

    CachedPrimitiveComponent = FindPrimitiveComponentByName(CachedOwnerActor, ComponentName);
    if (!CachedPrimitiveComponent)
    {
        UE_LOG_WARNING(
            "[AnimNotifyBuiltins] AttackWindow target component not found or not primitive | Notify=%s | Component=%s",
            GetNotifyLabel(NotifyEvent).c_str(),
            ComponentName.c_str());
        CachedOwnerActor = nullptr;
        return;
    }

    bWindowAcquired = AcquireAttackWindow(CachedPrimitiveComponent);

    ActiveAttackTag = BuildAttackIdTag(Payload.GetStringAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::AttackIdKey())));
    if (!ActiveAttackTag.empty())
    {
        ActiveAttackTagStateKey = AcquireAttackTagStateKey(CachedOwnerActor, ActiveAttackTag);
        bAttackTagAcquired = !ActiveAttackTagStateKey.empty();
    }
}

void UAnimNotifyState_AttackWindow::NotifyEnd(
    USkeletalMeshComponent* MeshComponent,
    UAnimSequence* Animation,
    const FAnimNotifyEvent& NotifyEvent)
{
    (void)MeshComponent;
    (void)Animation;
    (void)NotifyEvent;
    Cleanup();
}

void UAnimNotifyState_AttackWindow::Cleanup()
{
    if (bAttackTagAcquired && !ActiveAttackTag.empty())
    {
        ReleaseAttackTag(CachedOwnerActor, ActiveAttackTagStateKey, ActiveAttackTag);
    }

    if (bWindowAcquired)
    {
        ReleaseAttackWindow(CachedPrimitiveComponent);
    }

    CachedOwnerActor = nullptr;
    CachedPrimitiveComponent = nullptr;
    ActiveAttackTag.clear();
    ActiveAttackTagStateKey.clear();
    bWindowAcquired = false;
    bAttackTagAcquired = false;
}
