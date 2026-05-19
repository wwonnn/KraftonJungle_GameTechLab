#pragma once

#include "Core/CoreMinimal.h"
#include "Object/FName.h"

class FAnimNotifyPayloadParser
{
public:
    FAnimNotifyPayloadParser() = default;
    explicit FAnimNotifyPayloadParser(const FString& Payload);

    void Parse(const FString& Payload);
    const TMap<FString, FString>& GetValues() const { return Values; }

    bool HasValue(const FString& Key) const;
    bool HasAnyValue(const TArray<FString>& Keys) const;
    FString GetString(const FString& Key, const FString& DefaultValue = FString()) const;
    FString GetStringAny(const TArray<FString>& Keys, const FString& DefaultValue = FString()) const;
    FName GetName(const FString& Key, const FName& DefaultValue = FName::None) const;
    FName GetNameAny(const TArray<FString>& Keys, const FName& DefaultValue = FName::None) const;
    bool TryGetFloat(const FString& Key, float& OutValue) const;
    bool TryGetFloatAny(const TArray<FString>& Keys, float& OutValue) const;
    float GetFloat(const FString& Key, float DefaultValue) const;
    float GetFloatAny(const TArray<FString>& Keys, float DefaultValue) const;
    bool TryGetBool(const FString& Key, bool& OutValue) const;
    bool TryGetBoolAny(const TArray<FString>& Keys, bool& OutValue) const;
    bool GetBool(const FString& Key, bool DefaultValue) const;
    bool GetBoolAny(const TArray<FString>& Keys, bool DefaultValue) const;
    void SetString(const FString& Key, const FString& Value);
    void SetName(const FString& Key, const FName& Value);
    void SetFloat(const FString& Key, float Value);
    void SetBool(const FString& Key, bool Value);
    void RemoveValue(const FString& Key);
    void RemoveAny(const TArray<FString>& Keys);
    FString SerializeCanonical() const;

private:
    static FString NormalizeKey(const FString& Key);
    static FString Trim(const FString& Value);

private:
    TMap<FString, FString> Values;
    TMap<FString, FString> OriginalKeys;
};
