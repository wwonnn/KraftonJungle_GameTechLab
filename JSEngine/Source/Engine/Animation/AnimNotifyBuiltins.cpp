#include "Animation/AnimNotifyBuiltins.h"

#include "Animation/AnimData/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNotifyPayloadParser.h"
#include "Animation/AnimNotifySemanticFieldNames.h"
#include "Component/PrimitiveComponent.h"
#include "Component/SkeletalMeshComponent.h"
#include "Core/Logging/Log.h"
#include "Engine/Geometry/Ray.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"
#include "Render/Resource/Material.h"
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

    bool TryTraceDownwardSurfaceHit(
        USkeletalMeshComponent* MeshComponent,
        const FVector& StartLocation,
        float TraceDistance,
        FHitResult& OutHit)
    {
        OutHit.Reset();

        if (!MeshComponent || TraceDistance <= 0.0f)
        {
            return false;
        }

        AActor* OwnerActor = ResolveOwnerActor(MeshComponent);
        UWorld* World = OwnerActor ? OwnerActor->GetFocusedWorld() : nullptr;
        if (!World)
        {
            return false;
        }

        const FRay DownwardRay(StartLocation, FVector(0.0f, 0.0f, -1.0f));
        bool bFoundHit = false;
        float ClosestDistance = TraceDistance;

        for (AActor* Actor : World->GetActors())
        {
            if (!IsLiveActor(Actor) || Actor == OwnerActor || !Actor->IsVisible())
            {
                continue;
            }

            for (UPrimitiveComponent* PrimitiveComponent : Actor->GetPrimitiveComponents())
            {
                if (!IsLiveObject(PrimitiveComponent))
                {
                    continue;
                }

                FHitResult CandidateHit;
                if (!PrimitiveComponent->Raycast(DownwardRay, CandidateHit) || !CandidateHit.IsValid())
                {
                    continue;
                }

                if (CandidateHit.Distance < 0.0f || CandidateHit.Distance > ClosestDistance)
                {
                    continue;
                }

                ClosestDistance = CandidateHit.Distance;
                OutHit = CandidateHit;
                bFoundHit = true;
            }
        }

        return bFoundHit;
    }

    FString ResolveFootstepSurfaceName(const FHitResult& HitResult)
    {
        UPrimitiveComponent* PrimitiveComponent = HitResult.HitComponent;
        if (!IsLiveObject(PrimitiveComponent))
        {
            return "Default";
        }

        if (PrimitiveComponent->GetNumMaterials() > 0)
        {
            if (UMaterialInterface* Material = PrimitiveComponent->GetMaterial(0))
            {
                if (IsLiveObject(Material))
                {
                    const FString& MaterialName = Material->GetName();
                    if (!MaterialName.empty())
                    {
                        return MaterialName;
                    }
                }
            }
        }

        const FString ComponentName = PrimitiveComponent->GetName();
        if (!ComponentName.empty())
        {
            return ComponentName;
        }

        AActor* OwnerActor = PrimitiveComponent->GetOwner();
        if (IsLiveActor(OwnerActor))
        {
            const FString ActorName = OwnerActor->GetName();
            if (!ActorName.empty())
            {
                return ActorName;
            }
        }

        return "Default";
    }

    FVector ResolveDecalSurfaceNormal(const FHitResult& HitResult, bool bAlignToHitNormal)
    {
        if (bAlignToHitNormal && HitResult.IsValid())
        {
            FVector SurfaceNormal = HitResult.Normal;
            if (SurfaceNormal.NormalizeSafe())
            {
                return SurfaceNormal;
            }
        }

        return FVector::UpVector;
    }

    bool DispatchGameplayEventWindowPhase(
        USkeletalMeshComponent* MeshComponent,
        UAnimSequence* Animation,
        const FAnimNotifyEvent& NotifyEvent,
        bool bUseEndEventName)
    {
        if (!MeshComponent)
        {
            return false;
        }

        UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
        if (!AnimInstance)
        {
            return false;
        }

        const FAnimNotifyPayloadParser PayloadParser(NotifyEvent.Payload);
        const FString EventName = PayloadParser.GetStringAny(
            AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::EventNameKey()));
        if (EventName.empty())
        {
            UE_LOG_WARNING(
                "[AnimNotifyBuiltins] GameplayEventWindow notify missing EventName payload | Notify=%s",
                GetNotifyLabel(NotifyEvent).c_str());
            return false;
        }

        FString DispatchEventName = EventName;
        if (bUseEndEventName)
        {
            const FString EndEventName = PayloadParser.GetStringAny(
                AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::EndEventNameKey()));
            if (!EndEventName.empty())
            {
                DispatchEventName = EndEventName;
            }
        }

        const FString PayloadValue = PayloadParser.GetStringAny(
            AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::PayloadKey()));
        AnimInstance->DispatchGameplayAnimNotifyEvent(Animation, NotifyEvent, DispatchEventName, PayloadValue);
        return true;
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

void UAnimNotify_GameplayEvent::Notify(
    USkeletalMeshComponent* MeshComponent,
    UAnimSequence* Animation,
    const FAnimNotifyEvent& NotifyEvent)
{
    if (!MeshComponent)
    {
        return;
    }

    UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    const FAnimNotifyPayloadParser PayloadParser(NotifyEvent.Payload);
    const FString EventName = PayloadParser.GetStringAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::EventNameKey()));
    if (EventName.empty())
    {
        UE_LOG_WARNING("[AnimNotifyBuiltins] GameplayEvent notify missing EventName payload | Notify=%s", GetNotifyLabel(NotifyEvent).c_str());
        return;
    }

    const FString PayloadValue = PayloadParser.GetStringAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::PayloadKey()));
    AnimInstance->DispatchGameplayAnimNotifyEvent(Animation, NotifyEvent, EventName, PayloadValue);
}

void UAnimNotify_CameraShake::Notify(
    USkeletalMeshComponent* MeshComponent,
    UAnimSequence* Animation,
    const FAnimNotifyEvent& NotifyEvent)
{
    if (!MeshComponent)
    {
        return;
    }

    UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    const FAnimNotifyPayloadParser PayloadParser(NotifyEvent.Payload);
    const FString Shake = PayloadParser.GetStringAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::ShakeKey()));
    if (Shake.empty())
    {
        UE_LOG_WARNING("[AnimNotifyBuiltins] CameraShake notify missing Shake payload | Notify=%s", GetNotifyLabel(NotifyEvent).c_str());
        return;
    }

    const float Scale = PayloadParser.GetFloatAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::ScaleKey()), 1.0f);
    AnimInstance->DispatchCameraShakeAnimNotify(Animation, NotifyEvent, Shake, Scale);
}

void UAnimNotify_FootstepSurfaceEvent::Notify(
    USkeletalMeshComponent* MeshComponent,
    UAnimSequence* Animation,
    const FAnimNotifyEvent& NotifyEvent)
{
    if (!MeshComponent)
    {
        return;
    }

    UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    const FAnimNotifyPayloadParser PayloadParser(NotifyEvent.Payload);
    const FString EventName = PayloadParser.GetStringAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::EventNameKey()));
    if (EventName.empty())
    {
        UE_LOG_WARNING("[AnimNotifyBuiltins] FootstepSurfaceEvent notify missing EventName payload | Notify=%s", GetNotifyLabel(NotifyEvent).c_str());
        return;
    }

    const FName SocketName = PayloadParser.GetNameAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::SocketNameKey()));
    if (SocketName == FName::None)
    {
        UE_LOG_WARNING("[AnimNotifyBuiltins] FootstepSurfaceEvent notify missing SocketName payload | Notify=%s", GetNotifyLabel(NotifyEvent).c_str());
        return;
    }

    const float TraceDistance = PayloadParser.GetFloatAny(
        AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::TraceDistanceKey()),
        25.0f);
    const FString PayloadValue = PayloadParser.GetStringAny(AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::PayloadKey()));

    const FVector TraceStartLocation = ResolvePlaybackLocation(MeshComponent, SocketName);
    FHitResult SurfaceHit;
    const bool bHasSurfaceHit = TryTraceDownwardSurfaceHit(MeshComponent, TraceStartLocation, TraceDistance, SurfaceHit);
    const FString SurfaceName = bHasSurfaceHit ? ResolveFootstepSurfaceName(SurfaceHit) : FString("Default");
    const FVector HitLocation = bHasSurfaceHit ? SurfaceHit.Location : TraceStartLocation;

    AnimInstance->DispatchFootstepSurfaceAnimNotify(
        Animation,
        NotifyEvent,
        EventName,
        SurfaceName,
        PayloadValue,
        HitLocation);
}

void UAnimNotify_PlayVFX::Notify(
    USkeletalMeshComponent* MeshComponent,
    UAnimSequence* Animation,
    const FAnimNotifyEvent& NotifyEvent)
{
    if (!MeshComponent)
    {
        return;
    }

    UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    const FAnimNotifyPayloadParser PayloadParser(NotifyEvent.Payload);
    const FString Effect = PayloadParser.GetStringAny(
        AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::EffectKey()));
    if (Effect.empty())
    {
        UE_LOG_WARNING("[AnimNotifyBuiltins] PlayVFX notify missing Effect payload | Notify=%s", GetNotifyLabel(NotifyEvent).c_str());
        return;
    }

    const FName SocketName = PayloadParser.GetNameAny(
        AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::SocketNameKey()));
    const bool bAttached = PayloadParser.GetBoolAny(
        AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::AttachedKey()),
        false);
    const float Scale = PayloadParser.GetFloatAny(
        AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::ScaleKey()),
        1.0f);
    const FVector PlaybackLocation = ResolvePlaybackLocation(MeshComponent, SocketName);

    AnimInstance->DispatchPlayVFXAnimNotify(
        Animation,
        NotifyEvent,
        Effect,
        PlaybackLocation,
        SocketName,
        bAttached,
        Scale);
}

void UAnimNotify_SpawnDecal::Notify(
    USkeletalMeshComponent* MeshComponent,
    UAnimSequence* Animation,
    const FAnimNotifyEvent& NotifyEvent)
{
    if (!MeshComponent)
    {
        return;
    }

    UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    const FAnimNotifyPayloadParser PayloadParser(NotifyEvent.Payload);
    const FString Decal = PayloadParser.GetStringAny(
        AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::DecalKey()));
    if (Decal.empty())
    {
        UE_LOG_WARNING("[AnimNotifyBuiltins] SpawnDecal notify missing Decal payload | Notify=%s", GetNotifyLabel(NotifyEvent).c_str());
        return;
    }

    const FName SocketName = PayloadParser.GetNameAny(
        AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::SocketNameKey()));
    const float TraceDistance = PayloadParser.GetFloatAny(
        AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::TraceDistanceKey()),
        50.0f);
    const float Size = PayloadParser.GetFloatAny(
        AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::SizeKey()),
        1.0f);
    const float Lifetime = PayloadParser.GetFloatAny(
        AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::LifetimeKey()),
        0.0f);
    const bool bAlignToHitNormal = PayloadParser.GetBoolAny(
        AnimNotifySemanticFieldNames::GetLookupKeys(AnimNotifySemanticFieldNames::AlignToHitNormalKey()),
        true);

    const FVector SpawnOrigin = ResolvePlaybackLocation(MeshComponent, SocketName);
    FHitResult SurfaceHit;
    const bool bHasSurfaceHit = TryTraceDownwardSurfaceHit(MeshComponent, SpawnOrigin, TraceDistance, SurfaceHit);
    const FVector SpawnLocation = bHasSurfaceHit ? SurfaceHit.Location : SpawnOrigin;
    const FVector SpawnNormal = bHasSurfaceHit
        ? ResolveDecalSurfaceNormal(SurfaceHit, bAlignToHitNormal)
        : FVector::UpVector;

    AnimInstance->DispatchSpawnDecalAnimNotify(
        Animation,
        NotifyEvent,
        Decal,
        SpawnLocation,
        SpawnNormal,
        SocketName,
        Size,
        Lifetime,
        bAlignToHitNormal);
}

void UAnimNotifyState_GameplayEventWindow::NotifyBegin(
    USkeletalMeshComponent* MeshComponent,
    UAnimSequence* Animation,
    const FAnimNotifyEvent& NotifyEvent)
{
    DispatchGameplayEventWindowPhase(MeshComponent, Animation, NotifyEvent, false);
}

void UAnimNotifyState_GameplayEventWindow::NotifyEnd(
    USkeletalMeshComponent* MeshComponent,
    UAnimSequence* Animation,
    const FAnimNotifyEvent& NotifyEvent)
{
    DispatchGameplayEventWindowPhase(MeshComponent, Animation, NotifyEvent, true);
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
