#include "Object/Reflection.h"

#include "Object/Object.h"

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
