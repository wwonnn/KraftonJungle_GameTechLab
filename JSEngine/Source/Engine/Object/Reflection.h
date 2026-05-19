#pragma once

#include "Core/PropertyTypes.h"
#include "Core/Containers/Array.h"

#include <utility>

class UObject;
struct FArchive;

#define UPROPERTY(...)
#define USTRUCT(...)
#define UENUM(...)
#define UMETA(...)
#define GENERATED_BODY(...)

struct FEnumValue
{
    const char* Name = nullptr;
    int64 Value = 0;
};

class UEnum
{
public:
    UEnum() = default;
    UEnum(
        const char* InName,
        const char* const* InNames,
        const FEnumValue* InValues,
        uint32 InValueCount)
        : Name(InName)
        , Names(InNames)
        , Values(InValues)
        , ValueCount(InValueCount)
    {
    }

    const char* GetName() const { return Name; }
    const char* const* GetNames() const { return Names; }
    const FEnumValue* GetValues() const { return Values; }
    uint32 GetValueCount() const { return ValueCount; }

private:
    const char* Name = nullptr;
    const char* const* Names = nullptr;
    const FEnumValue* Values = nullptr;
    uint32 ValueCount = 0;
};

class UStruct
{
public:
    UStruct() = default;
    UStruct(
        const char* InName,
        size_t InSize,
        const FProperty* const* InProperties,
        uint32 InPropertyCount)
        : Name(InName)
        , Size(InSize)
        , Properties(InProperties)
        , PropertyCount(InPropertyCount)
    {
    }

    const char* GetName() const { return Name; }
    size_t GetSize() const { return Size; }
    const FProperty* const* GetProperties() const { return Properties; }
    uint32 GetPropertyCount() const { return PropertyCount; }

    void AppendProperties(void* StructValue, TArray<FPropertyDescriptor>& OutProps) const;
    void SerializeProperties(FArchive& Ar, UObject* OwnerObject, void* StructValue) const;

private:
    const char* Name = nullptr;
    size_t Size = 0;
    const FProperty* const* Properties = nullptr;
    uint32 PropertyCount = 0;
};

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
