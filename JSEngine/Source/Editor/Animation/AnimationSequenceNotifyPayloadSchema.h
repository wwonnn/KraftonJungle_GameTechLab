#pragma once

#include "Core/CoreMinimal.h"

enum class EAnimNotifyPayloadFieldType : uint8
{
    String = 0,
    Name,
    Float,
    Bool,
};

struct FAnimNotifyPayloadFieldDefinition
{
    FString Key;
    FString Label;
    EAnimNotifyPayloadFieldType Type = EAnimNotifyPayloadFieldType::String;
    bool bRequired = false;
    FString DefaultValue;
    FString HelpText;
};

struct FAnimNotifyPayloadSchema
{
    FString NotifyClassName;
    FString SectionLabel;
    FString PayloadExample;
    TArray<FAnimNotifyPayloadFieldDefinition> Fields;
};

const FAnimNotifyPayloadSchema* FindAnimNotifyPayloadSchema(const FString& NotifyClassName);
bool HasAnimNotifyPayloadSchema(const FString& NotifyClassName);
const char* GetAnimNotifyPayloadExample(const FString& NotifyClassName);
