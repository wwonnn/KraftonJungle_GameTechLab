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

void FBoolProperty::SerializeItem(FArchive& Ar, UObject* OwnerObject, void* Container) const
{
    (void)OwnerObject;
    Ar << GetSerializeKey() << *static_cast<bool*>(ContainerPtrToValuePtr(Container));
}

void FIntProperty::SerializeItem(FArchive& Ar, UObject* OwnerObject, void* Container) const
{
    (void)OwnerObject;
    Ar << GetSerializeKey() << *static_cast<int32*>(ContainerPtrToValuePtr(Container));
}

void FFloatProperty::SerializeItem(FArchive& Ar, UObject* OwnerObject, void* Container) const
{
    (void)OwnerObject;
    Ar << GetSerializeKey() << *static_cast<float*>(ContainerPtrToValuePtr(Container));
}

void FVectorProperty::SerializeItem(FArchive& Ar, UObject* OwnerObject, void* Container) const
{
    (void)OwnerObject;
    Ar << GetSerializeKey() << *static_cast<FVector*>(ContainerPtrToValuePtr(Container));
}

void FVector4Property::SerializeItem(FArchive& Ar, UObject* OwnerObject, void* Container) const
{
    (void)OwnerObject;
    Ar << GetSerializeKey() << *static_cast<FVector4*>(ContainerPtrToValuePtr(Container));
}

void FStringProperty::SerializeItem(FArchive& Ar, UObject* OwnerObject, void* Container) const
{
    (void)OwnerObject;
    Ar << GetSerializeKey() << *static_cast<FString*>(ContainerPtrToValuePtr(Container));
}

void FNameProperty::SerializeItem(FArchive& Ar, UObject* OwnerObject, void* Container) const
{
    (void)OwnerObject;
    Ar << GetSerializeKey() << *static_cast<FName*>(ContainerPtrToValuePtr(Container));
}

void FColorProperty::SerializeItem(FArchive& Ar, UObject* OwnerObject, void* Container) const
{
    (void)OwnerObject;
    Ar << GetSerializeKey() << *static_cast<FColor*>(ContainerPtrToValuePtr(Container));
}

void FObjectProperty::SerializeItem(FArchive& Ar, UObject* OwnerObject, void* Container) const
{
    (void)OwnerObject;
    UObject*& Ref = *static_cast<UObject**>(ContainerPtrToValuePtr(Container));
    uint32 RefUUID = Ref ? Ref->GetUUID() : 0;
    Ar << GetSerializeKey() << RefUUID;
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
}

void FSceneComponentProperty::SerializeItem(FArchive& Ar, UObject* OwnerObject, void* Container) const
{
    USceneComponent*& Ref = *static_cast<USceneComponent**>(ContainerPtrToValuePtr(Container));
    uint32 RefUUID = Ref ? Ref->GetUUID() : 0;
    Ar << GetSerializeKey() << RefUUID;
    if (Ar.IsLoading())
    {
        Ref = nullptr;
        UActorComponent* OwnerComponent = Cast<UActorComponent>(OwnerObject);
        AActor* Owner = OwnerComponent ? OwnerComponent->GetOwner() : nullptr;
        if (!Owner)
        {
            return;
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
}

void FReflectionRegistry::RegisterProperties(
    const FTypeInfo* Type,
    const FProperty* const* Properties,
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

    for (uint32 Index = 0; Index < Registered->PropertyCount; ++Index)
    {
        const FProperty* Property = Registered->Properties[Index];
        if (!Property || !Property->GetName())
        {
            continue;
        }

        OutProps.push_back({
            Property->GetName(),
            Property->GetType(),
            Property->ContainerPtrToValuePtr(Object),
            Property->GetMin(),
            Property->GetMax(),
            Property->GetSpeed(),
            nullptr,
            0,
            nullptr,
            Property->GetDescriptorUsageFlags(),
            Property->GetObjectType(),
            Property->GetDisplayName()
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

    for (uint32 Index = 0; Index < Registered->PropertyCount; ++Index)
    {
        const FProperty* Property = Registered->Properties[Index];
        if (!Property || !Property->GetName() || !Property->ShouldSerialize())
        {
            continue;
        }

        const char* Key = Property->GetSerializeKey();
        if (!Key)
        {
            continue;
        }

        if (Ar.IsLoading() && !Ar.HasKey(Key))
        {
            continue;
        }

        Property->SerializeItem(Ar, Object, Object);
    }
}
