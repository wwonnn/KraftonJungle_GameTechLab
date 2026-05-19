#pragma once

#include "Core/Containers/Array.h"
#include "Reflection/Property.h"
#include "Reflection/Enum.h"
#include "Reflection/Struct.h"

#include <utility>

class UObject;
struct FArchive;

#define UPROPERTY(...)
#define USTRUCT(...)
#define UENUM(...)
#define UMETA(...)

#define REFLECTION_BODY_MACRO_COMBINE_INNER(A, B, C, D) A##B##C##D
#define REFLECTION_BODY_MACRO_COMBINE(A, B, C, D) REFLECTION_BODY_MACRO_COMBINE_INNER(A, B, C, D)
#define GENERATED_BODY(...) REFLECTION_BODY_MACRO_COMBINE(CURRENT_FILE_ID, _, __LINE__, _GENERATED_BODY)

class FReflectionRegistry
{
public:
    static FReflectionRegistry& Get();

    void RegisterProperties(
        const FTypeInfo* Type,
        const FProperty* const* Properties,
        uint32 PropertyCount);

    void AppendProperties(
        const FTypeInfo* Type,
        UObject* Object,
        TArray<FPropertyDescriptor>& OutProps) const;

    void SerializeProperties(
        const FTypeInfo* Type,
        UObject* Object,
        FArchive& Ar) const;

private:
    struct FRegisteredProperties
    {
        const FProperty* const* Properties = nullptr;
        uint32 PropertyCount = 0;
    };

    TArray<std::pair<const FTypeInfo*, FRegisteredProperties>> RegisteredTypes;
};
