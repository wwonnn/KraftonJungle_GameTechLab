#pragma once

#include "Core/PropertyTypes.h"
#include "Core/Containers/Array.h"

#include <utility>

class UObject;

#define UPROPERTY(...)

class FReflectionRegistry
{
public:
    static FReflectionRegistry& Get();

    void RegisterProperties(
        const FTypeInfo* Type,
        const FPropertyMeta* Properties,
        uint32 PropertyCount);

    void AppendProperties(
        const FTypeInfo* Type,
        UObject* Object,
        TArray<FPropertyDescriptor>& OutProps) const;

private:
    struct FRegisteredProperties
    {
        const FPropertyMeta* Properties = nullptr;
        uint32 PropertyCount = 0;
    };

    TArray<std::pair<const FTypeInfo*, FRegisteredProperties>> RegisteredTypes;
};
