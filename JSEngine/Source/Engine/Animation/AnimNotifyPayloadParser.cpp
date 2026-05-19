#include "Animation/AnimNotifyPayloadParser.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <utility>

namespace
{
    FString ToLowerAscii(FString Value)
    {
        std::transform(
            Value.begin(),
            Value.end(),
            Value.begin(),
            [](unsigned char Character)
            {
                return static_cast<char>(std::tolower(Character));
            });
        return Value;
    }

    FString FormatFloatCanonical(float Value)
    {
        char Buffer[64] = {};
        snprintf(Buffer, sizeof(Buffer), "%.9g", static_cast<double>(Value));
        return FString(Buffer);
    }
}

FAnimNotifyPayloadParser::FAnimNotifyPayloadParser(const FString& Payload)
{
    Parse(Payload);
}

void FAnimNotifyPayloadParser::Parse(const FString& Payload)
{
    Values.clear();
    OriginalKeys.clear();

    size_t Cursor = 0;
    while (Cursor < Payload.size())
    {
        const size_t SeparatorIndex = Payload.find(';', Cursor);
        const FString Entry = SeparatorIndex == FString::npos
            ? Payload.substr(Cursor)
            : Payload.substr(Cursor, SeparatorIndex - Cursor);
        Cursor = SeparatorIndex == FString::npos ? Payload.size() : SeparatorIndex + 1;

        const FString TrimmedEntry = Trim(Entry);
        if (TrimmedEntry.empty())
        {
            continue;
        }

        const size_t EqualsIndex = TrimmedEntry.find('=');
        if (EqualsIndex == FString::npos)
        {
            continue;
        }

        const FString OriginalKey = Trim(TrimmedEntry.substr(0, EqualsIndex));
        const FString Key = NormalizeKey(OriginalKey);
        if (Key.empty())
        {
            continue;
        }

        OriginalKeys[Key] = OriginalKey;
        Values[Key] = Trim(TrimmedEntry.substr(EqualsIndex + 1));
    }
}

bool FAnimNotifyPayloadParser::HasValue(const FString& Key) const
{
    return Values.find(NormalizeKey(Key)) != Values.end();
}

FString FAnimNotifyPayloadParser::GetString(const FString& Key, const FString& DefaultValue) const
{
    const auto Iterator = Values.find(NormalizeKey(Key));
    return Iterator != Values.end() ? Iterator->second : DefaultValue;
}

FName FAnimNotifyPayloadParser::GetName(const FString& Key, const FName& DefaultValue) const
{
    const FString Value = GetString(Key);
    return Value.empty() ? DefaultValue : FName(Value);
}

bool FAnimNotifyPayloadParser::TryGetFloat(const FString& Key, float& OutValue) const
{
    const FString Value = GetString(Key);
    if (Value.empty())
    {
        return false;
    }

    char* ParseEnd = nullptr;
    const float ParsedValue = std::strtof(Value.c_str(), &ParseEnd);
    if (ParseEnd == Value.c_str())
    {
        return false;
    }

    while (ParseEnd && *ParseEnd != '\0')
    {
        if (!std::isspace(static_cast<unsigned char>(*ParseEnd)))
        {
            return false;
        }
        ++ParseEnd;
    }

    OutValue = ParsedValue;
    return true;
}

float FAnimNotifyPayloadParser::GetFloat(const FString& Key, float DefaultValue) const
{
    float ParsedValue = DefaultValue;
    return TryGetFloat(Key, ParsedValue) ? ParsedValue : DefaultValue;
}

bool FAnimNotifyPayloadParser::TryGetBool(const FString& Key, bool& OutValue) const
{
    const FString Value = ToLowerAscii(GetString(Key));
    if (Value.empty())
    {
        return false;
    }

    if (Value == "1" || Value == "true" || Value == "yes" || Value == "on")
    {
        OutValue = true;
        return true;
    }

    if (Value == "0" || Value == "false" || Value == "no" || Value == "off")
    {
        OutValue = false;
        return true;
    }

    return false;
}

bool FAnimNotifyPayloadParser::GetBool(const FString& Key, bool DefaultValue) const
{
    bool ParsedValue = DefaultValue;
    return TryGetBool(Key, ParsedValue) ? ParsedValue : DefaultValue;
}

void FAnimNotifyPayloadParser::SetString(const FString& Key, const FString& Value)
{
    const FString NormalizedKey = NormalizeKey(Key);
    if (NormalizedKey.empty())
    {
        return;
    }

    const FString TrimmedValue = Trim(Value);
    if (TrimmedValue.empty())
    {
        Values.erase(NormalizedKey);
        OriginalKeys.erase(NormalizedKey);
        return;
    }

    OriginalKeys[NormalizedKey] = Trim(Key);
    Values[NormalizedKey] = TrimmedValue;
}

void FAnimNotifyPayloadParser::SetName(const FString& Key, const FName& Value)
{
    SetString(Key, Value.IsValid() ? Value.ToString() : FString());
}

void FAnimNotifyPayloadParser::SetFloat(const FString& Key, float Value)
{
    SetString(Key, FormatFloatCanonical(Value));
}

void FAnimNotifyPayloadParser::SetBool(const FString& Key, bool Value)
{
    SetString(Key, Value ? "true" : "false");
}

void FAnimNotifyPayloadParser::RemoveValue(const FString& Key)
{
    const FString NormalizedKey = NormalizeKey(Key);
    Values.erase(NormalizedKey);
    OriginalKeys.erase(NormalizedKey);
}

FString FAnimNotifyPayloadParser::SerializeCanonical() const
{
    if (Values.empty())
    {
        return FString();
    }

    struct FSerializedEntry
    {
        FString NormalizedKey;
        FString OutputKey;
        FString Value;
    };

    TArray<FSerializedEntry> SortedEntries;
    SortedEntries.reserve(Values.size());
    for (const auto& [Key, Value] : Values)
    {
        if (!Key.empty() && !Value.empty())
        {
            const auto OriginalKeyIterator = OriginalKeys.find(Key);
            SortedEntries.push_back(
                {
                    Key,
                    OriginalKeyIterator != OriginalKeys.end() && !OriginalKeyIterator->second.empty()
                        ? OriginalKeyIterator->second
                        : Key,
                    Value
                });
        }
    }

    std::sort(
        SortedEntries.begin(),
        SortedEntries.end(),
        [](const FSerializedEntry& Left, const FSerializedEntry& Right)
        {
            return Left.NormalizedKey < Right.NormalizedKey;
        });

    FString Result;
    for (size_t Index = 0; Index < SortedEntries.size(); ++Index)
    {
        if (Index > 0)
        {
            Result += ";";
        }

        Result += SortedEntries[Index].OutputKey;
        Result += "=";
        Result += SortedEntries[Index].Value;
    }

    return Result;
}

FString FAnimNotifyPayloadParser::NormalizeKey(const FString& Key)
{
    return ToLowerAscii(Trim(Key));
}

FString FAnimNotifyPayloadParser::Trim(const FString& Value)
{
    size_t Start = 0;
    while (Start < Value.size() && std::isspace(static_cast<unsigned char>(Value[Start])))
    {
        ++Start;
    }

    size_t End = Value.size();
    while (End > Start && std::isspace(static_cast<unsigned char>(Value[End - 1])))
    {
        --End;
    }

    return Value.substr(Start, End - Start);
}
