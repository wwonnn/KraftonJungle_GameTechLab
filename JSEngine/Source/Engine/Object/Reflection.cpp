#include "Object/Reflection.h"

#include "Component/ActorComponent.h"
#include "Component/SceneComponent.h"
#include "GameFramework/AActor.h"
#include "Object/Object.h"
#include "Serialization/Archive.h"

FReflectionRegistry& FReflectionRegistry::Get()
{
    static FReflectionRegistry Registry;
    return Registry;
}

void FReflectionRegistry::RegisterProperties(
    const FTypeInfo* Type,
    const FPropertyMeta* Properties,
    uint32 PropertyCount)
{
    if (!Type)
    {
        return;
    }

    for (auto& Entry : RegisteredTypes)
    {
        if (Entry.first == Type)
        {
            Entry.second.Properties = Properties;
            Entry.second.PropertyCount = PropertyCount;
            return;
        }
    }

    RegisteredTypes.push_back({ Type, { Properties, PropertyCount } });
}

void FReflectionRegistry::AppendProperties(
    const FTypeInfo* Type,
    UObject* Object,
    TArray<FPropertyDescriptor>& OutProps) const
{
    if (!Type || !Object)
    {
        return;
    }

    if (Type->Parent)
    {
        AppendProperties(Type->Parent, Object, OutProps);
    }

    const FRegisteredProperties* Registered = nullptr;
    for (const auto& Entry : RegisteredTypes)
    {
        if (Entry.first == Type)
        {
            Registered = &Entry.second;
            break;
        }
    }

    if (!Registered || !Registered->Properties)
    {
        return;
    }

    uint8* ObjectBytes = reinterpret_cast<uint8*>(Object);
    for (uint32 Index = 0; Index < Registered->PropertyCount; ++Index)
    {
        const FPropertyMeta& Meta = Registered->Properties[Index];
        if (!Meta.Name)
        {
            continue;
        }

        const EPropertyUsageFlags BaseUsage =
            Meta.Access == EPropertyAccess::EditAnywhere
                ? (EPropertyUsageFlags::Visible | EPropertyUsageFlags::Editable)
                : EPropertyUsageFlags::Visible;
        const EPropertyUsageFlags Usage = BaseUsage | Meta.UsageFlags;

        OutProps.push_back({
            Meta.Name,
            Meta.Type,
            ObjectBytes + Meta.Offset,
            Meta.Min,
            Meta.Max,
            Meta.Speed,
            nullptr,
            0,
            nullptr,
            Usage,
            Meta.ObjectType,
            Meta.DisplayName
        });
    }
}

void FReflectionRegistry::SerializeProperties(
    const FTypeInfo* Type,
    UObject* Object,
    FArchive& Ar) const
{
    if (!Type || !Object)
    {
        return;
    }

    if (Type->Parent)
    {
        SerializeProperties(Type->Parent, Object, Ar);
    }

    const FRegisteredProperties* Registered = nullptr;
    for (const auto& Entry : RegisteredTypes)
    {
        if (Entry.first == Type)
        {
            Registered = &Entry.second;
            break;
        }
    }

    if (!Registered || !Registered->Properties)
    {
        return;
    }

    uint8* ObjectBytes = reinterpret_cast<uint8*>(Object);
    for (uint32 Index = 0; Index < Registered->PropertyCount; ++Index)
    {
        const FPropertyMeta& Meta = Registered->Properties[Index];
        if (!Meta.Name || HasPropertyUsage(Meta.UsageFlags, EPropertyUsageFlags::NonSerialized))
        {
            continue;
        }

        const char* Key = Meta.SerializeName
            ? Meta.SerializeName
            : (Meta.DisplayName ? Meta.DisplayName : Meta.Name);
        if (!Key)
        {
            continue;
        }

        if (Ar.IsLoading() && !Ar.HasKey(Key))
        {
            continue;
        }

        void* ValuePtr = ObjectBytes + Meta.Offset;
        switch (Meta.Type)
        {
        case EPropertyType::Bool:
            Ar << Key << *static_cast<bool*>(ValuePtr);
            break;
        case EPropertyType::Int:
            Ar << Key << *static_cast<int32*>(ValuePtr);
            break;
        case EPropertyType::Float:
            Ar << Key << *static_cast<float*>(ValuePtr);
            break;
        case EPropertyType::Vec3:
            Ar << Key << *static_cast<FVector*>(ValuePtr);
            break;
        case EPropertyType::Vec4:
            Ar << Key << *static_cast<FVector4*>(ValuePtr);
            break;
        case EPropertyType::String:
            Ar << Key << *static_cast<FString*>(ValuePtr);
            break;
        case EPropertyType::Name:
            Ar << Key << *static_cast<FName*>(ValuePtr);
            break;
        case EPropertyType::Color:
            Ar << Key << *static_cast<FColor*>(ValuePtr);
            break;
        case EPropertyType::ObjectRef:
        {
            UObject*& Ref = *static_cast<UObject**>(ValuePtr);
            uint32 RefUUID = Ref ? Ref->GetUUID() : 0;
            Ar << Key << RefUUID;
            if (Ar.IsLoading())
            {
                Ref = nullptr;
                for (UObject* ObjectCandidate : GUObjectArray)
                {
                    if (ObjectCandidate && ObjectCandidate->GetUUID() == RefUUID)
                    {
                        Ref = ObjectCandidate;
                        break;
                    }
                }
            }
            break;
        }
        case EPropertyType::SceneComponentRef:
        {
            USceneComponent*& Ref = *static_cast<USceneComponent**>(ValuePtr);
            uint32 RefUUID = Ref ? Ref->GetUUID() : 0;
            Ar << Key << RefUUID;
            if (Ar.IsLoading())
            {
                Ref = nullptr;
                UActorComponent* OwnerComponent = Cast<UActorComponent>(Object);
                AActor* Owner = OwnerComponent ? OwnerComponent->GetOwner() : nullptr;
                if (!Owner)
                {
                    break;
                }

                for (UActorComponent* Component : Owner->GetComponents())
                {
                    USceneComponent* SceneComponent = Cast<USceneComponent>(Component);
                    if (SceneComponent && SceneComponent->GetUUID() == RefUUID)
                    {
                        Ref = SceneComponent;
                        break;
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }
}
