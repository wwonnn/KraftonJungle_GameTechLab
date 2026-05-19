#include "Asset/AnimSequenceAssetLoader.h"

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Core/Logging/Log.h"
#include "Core/Paths.h"
#include "Object/ObjectFactory.h"
#include "SimpleJSON/json.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace
{
    FString NormalizeSequencePath(const FString& Path)
    {
        return FPaths::ToProjectRelativePath(Path);
    }

    bool IsSequenceAssetPath(const FString& Path)
    {
        FString LowerPath = FPaths::Normalize(Path);
        std::transform(
            LowerPath.begin(),
            LowerPath.end(),
            LowerPath.begin(),
            [](unsigned char Ch)
            {
                return static_cast<char>(std::tolower(Ch));
            });

        return std::filesystem::path(FPaths::ToWide(LowerPath)).extension() == L".sequence";
    }

    json::JSON MakeVectorKey(const FVector& Value)
    {
        return json::Array(Value.X, Value.Y, Value.Z);
    }

    json::JSON MakeQuatKey(const FQuat& Value)
    {
        return json::Array(Value.X, Value.Y, Value.Z, Value.W);
    }

    FVector ReadVectorKey(const json::JSON& Value)
    {
        if (Value.JSONType() != json::JSON::Class::Array || Value.length() < 3)
        {
            return FVector::ZeroVector;
        }

        return FVector(
            static_cast<float>(Value.at(0).ToFloat()),
            static_cast<float>(Value.at(1).ToFloat()),
            static_cast<float>(Value.at(2).ToFloat()));
    }

    FQuat ReadQuatKey(const json::JSON& Value)
    {
        if (Value.JSONType() != json::JSON::Class::Array || Value.length() < 4)
        {
            return FQuat::Identity;
        }

        return FQuat(
            static_cast<float>(Value.at(0).ToFloat()),
            static_cast<float>(Value.at(1).ToFloat()),
            static_cast<float>(Value.at(2).ToFloat()),
            static_cast<float>(Value.at(3).ToFloat()));
    }

    void WriteVectorKeys(json::JSON& OutArray, const TArray<FVector>& Keys)
    {
        OutArray = json::Array();
        for (const FVector& Key : Keys)
        {
            OutArray.append(MakeVectorKey(Key));
        }
    }

    void WriteQuatKeys(json::JSON& OutArray, const TArray<FQuat>& Keys)
    {
        OutArray = json::Array();
        for (const FQuat& Key : Keys)
        {
            OutArray.append(MakeQuatKey(Key));
        }
    }

    void WriteFloatArray(json::JSON& OutArray, const TArray<float>& Values)
    {
        OutArray = json::Array();
        for (float Value : Values)
        {
            OutArray.append(Value);
        }
    }

    void WriteIntArray(json::JSON& OutArray, const TArray<int32>& Values)
    {
        OutArray = json::Array();
        for (int32 Value : Values)
        {
            OutArray.append(Value);
        }
    }

    void ReadVectorKeys(const json::JSON& SourceArray, TArray<FVector>& OutKeys)
    {
        OutKeys.clear();
        if (SourceArray.JSONType() != json::JSON::Class::Array)
        {
            return;
        }

        OutKeys.reserve(SourceArray.length());
        for (int32 Index = 0; Index < SourceArray.length(); ++Index)
        {
            OutKeys.push_back(ReadVectorKey(SourceArray.at(static_cast<unsigned>(Index))));
        }
    }

    void ReadQuatKeys(const json::JSON& SourceArray, TArray<FQuat>& OutKeys)
    {
        OutKeys.clear();
        if (SourceArray.JSONType() != json::JSON::Class::Array)
        {
            return;
        }

        OutKeys.reserve(SourceArray.length());
        for (int32 Index = 0; Index < SourceArray.length(); ++Index)
        {
            OutKeys.push_back(ReadQuatKey(SourceArray.at(static_cast<unsigned>(Index))));
        }
    }

    void ReadFloatArray(const json::JSON& SourceArray, TArray<float>& OutValues)
    {
        OutValues.clear();
        if (SourceArray.JSONType() != json::JSON::Class::Array)
        {
            return;
        }

        OutValues.reserve(SourceArray.length());
        for (int32 Index = 0; Index < SourceArray.length(); ++Index)
        {
            OutValues.push_back(static_cast<float>(SourceArray.at(static_cast<unsigned>(Index)).ToFloat()));
        }
    }

    void ReadIntArray(const json::JSON& SourceArray, TArray<int32>& OutValues)
    {
        OutValues.clear();
        if (SourceArray.JSONType() != json::JSON::Class::Array)
        {
            return;
        }

        OutValues.reserve(SourceArray.length());
        for (int32 Index = 0; Index < SourceArray.length(); ++Index)
        {
            OutValues.push_back(static_cast<int32>(SourceArray.at(static_cast<unsigned>(Index)).ToInt()));
        }
    }

    void WriteFloatCurve(json::JSON& OutObject, const FFloatCurve& Curve)
    {
        OutObject = json::JSON::Make(json::JSON::Class::Object);
        OutObject["CurveName"] = Curve.CurveName.ToString();
        OutObject["CurveType"] = AnimationCurveTypeToString(Curve.CurveType);
        OutObject["SourceKind"] = AnimationCurveSourceKindToString(Curve.SourceKind);

        TArray<float> Times;
        TArray<float> Values;
        TArray<int32> InterpModes;
        TArray<int32> TangentModes;
        TArray<float> ArriveTangents;
        TArray<float> LeaveTangents;

        Times.reserve(Curve.Keys.size());
        Values.reserve(Curve.Keys.size());
        InterpModes.reserve(Curve.Keys.size());
        TangentModes.reserve(Curve.Keys.size());
        ArriveTangents.reserve(Curve.Keys.size());
        LeaveTangents.reserve(Curve.Keys.size());

        for (const FCurveKey& Key : Curve.Keys)
        {
            Times.push_back(Key.Time);
            Values.push_back(Key.Value);
            InterpModes.push_back(static_cast<int32>(Key.InterpMode));
            TangentModes.push_back(static_cast<int32>(Key.TangentMode));
            ArriveTangents.push_back(Key.ArriveTangent);
            LeaveTangents.push_back(Key.LeaveTangent);
        }

        WriteFloatArray(OutObject["Times"], Times);
        WriteFloatArray(OutObject["Values"], Values);
        WriteIntArray(OutObject["InterpModes"], InterpModes);
        WriteIntArray(OutObject["TangentModes"], TangentModes);
        WriteFloatArray(OutObject["ArriveTangents"], ArriveTangents);
        WriteFloatArray(OutObject["LeaveTangents"], LeaveTangents);
    }

    FFloatCurve ReadFloatCurve(const json::JSON& SourceObject)
    {
        FFloatCurve Curve;
        if (SourceObject.JSONType() != json::JSON::Class::Object)
        {
            return Curve;
        }

        FString CurveName;
        if (SourceObject.hasKey("CurveName"))
        {
            CurveName = SourceObject.at("CurveName").ToString();
        }
        else if (SourceObject.hasKey("Name"))
        {
            CurveName = SourceObject.at("Name").ToString();
        }
        Curve.CurveName = FName(CurveName);
        if (SourceObject.hasKey("CurveType"))
        {
            Curve.CurveType = AnimationCurveTypeFromString(SourceObject.at("CurveType").ToString());
        }
        if (SourceObject.hasKey("SourceKind"))
        {
            Curve.SourceKind = AnimationCurveSourceKindFromString(SourceObject.at("SourceKind").ToString());
        }

        TArray<float> Times;
        TArray<float> Values;
        TArray<int32> InterpModes;
        TArray<int32> TangentModes;
        TArray<float> ArriveTangents;
        TArray<float> LeaveTangents;

        if (SourceObject.hasKey("Times"))
        {
            ReadFloatArray(SourceObject.at("Times"), Times);
        }
        if (SourceObject.hasKey("Values"))
        {
            ReadFloatArray(SourceObject.at("Values"), Values);
        }
        if (SourceObject.hasKey("InterpModes"))
        {
            ReadIntArray(SourceObject.at("InterpModes"), InterpModes);
        }
        if (SourceObject.hasKey("TangentModes"))
        {
            ReadIntArray(SourceObject.at("TangentModes"), TangentModes);
        }
        if (SourceObject.hasKey("ArriveTangents"))
        {
            ReadFloatArray(SourceObject.at("ArriveTangents"), ArriveTangents);
        }
        if (SourceObject.hasKey("LeaveTangents"))
        {
            ReadFloatArray(SourceObject.at("LeaveTangents"), LeaveTangents);
        }

        Curve.Keys.reserve(Times.size());
        for (size_t Index = 0; Index < Times.size(); ++Index)
        {
            FCurveKey Key;
            Key.Time = Times[Index];
            Key.Value = Index < Values.size() ? Values[Index] : 0.0f;
            Key.InterpMode = Index < InterpModes.size()
                ? static_cast<ECurveInterpMode>(InterpModes[Index])
                : ECurveInterpMode::Cubic;
            Key.TangentMode = Index < TangentModes.size()
                ? static_cast<ECurveTangentMode>(TangentModes[Index])
                : ECurveTangentMode::Auto;
            Key.ArriveTangent = Index < ArriveTangents.size() ? ArriveTangents[Index] : 0.0f;
            Key.LeaveTangent = Index < LeaveTangents.size() ? LeaveTangents[Index] : 0.0f;
            Curve.Keys.push_back(Key);
        }
        Curve.SortKeys();

        return Curve;
    }

    json::JSON MakeColorKey(const FColor& Value)
    {
        return json::Array(Value.R, Value.G, Value.B, Value.A);
    }

    FColor ReadColorKey(const json::JSON& Value)
    {
        if (Value.JSONType() != json::JSON::Class::Array || Value.length() < 4)
        {
            return FColor(0.9490196f, 0.61960787f, 0.23921569f, 1.0f);
        }

        const float Red = static_cast<float>(Value.at(0).ToFloat());
        const float Green = static_cast<float>(Value.at(1).ToFloat());
        const float Blue = static_cast<float>(Value.at(2).ToFloat());
        const float Alpha = static_cast<float>(Value.at(3).ToFloat());

        if (Red > 1.0f || Green > 1.0f || Blue > 1.0f || Alpha > 1.0f)
        {
            return FColor(
                static_cast<uint32>(std::clamp(Red, 0.0f, 255.0f)),
                static_cast<uint32>(std::clamp(Green, 0.0f, 255.0f)),
                static_cast<uint32>(std::clamp(Blue, 0.0f, 255.0f)),
                static_cast<uint32>(std::clamp(Alpha, 0.0f, 255.0f)));
        }

        return FColor(Red, Green, Blue, Alpha);
    }

    void WriteNotifyTrack(json::JSON& OutObject, const FAnimNotifyTrack& Track)
    {
        OutObject = json::JSON::Make(json::JSON::Class::Object);
        OutObject["TrackName"] = Track.TrackName.ToString();
        OutObject["Events"] = json::Array();

        for (const FAnimNotifyEvent& Event : Track.Events)
        {
            json::JSON EventObject = json::JSON::Make(json::JSON::Class::Object);
            EventObject["StableId"] = Event.StableId.ToString();
            EventObject["Name"] = Event.Name.ToString();
            EventObject["Time"] = Event.Time;
            EventObject["Duration"] = Event.Duration;
            EventObject["Color"] = MakeColorKey(Event.Color);
            EventObject["EventType"] = AnimNotifyEventTypeToString(Event.EventType);
            EventObject["NotifyClassName"] = Event.GetResolvedNotifyClassName();
            EventObject["Payload"] = Event.Payload;
            OutObject["Events"].append(EventObject);
        }
    }

    FAnimNotifyTrack ReadNotifyTrack(const json::JSON& SourceObject)
    {
        FAnimNotifyTrack Track;
        if (SourceObject.JSONType() != json::JSON::Class::Object)
        {
            return Track;
        }

        if (SourceObject.hasKey("TrackName"))
        {
            Track.TrackName = FName(SourceObject.at("TrackName").ToString());
        }
        else if (SourceObject.hasKey("Name"))
        {
            Track.TrackName = FName(SourceObject.at("Name").ToString());
        }

        if (!SourceObject.hasKey("Events") || SourceObject.at("Events").JSONType() != json::JSON::Class::Array)
        {
            return Track;
        }

        const json::JSON& Events = SourceObject.at("Events");
        Track.Events.reserve(Events.length());
        for (int32 EventIndex = 0; EventIndex < Events.length(); ++EventIndex)
        {
            const json::JSON& EventObject = Events.at(static_cast<unsigned>(EventIndex));
            if (EventObject.JSONType() != json::JSON::Class::Object)
            {
                continue;
            }

            FAnimNotifyEvent Event;
            if (EventObject.hasKey("StableId"))
            {
                Event.StableId = FGuid::FromString(EventObject.at("StableId").ToString());
            }
            if (EventObject.hasKey("Name"))
            {
                Event.Name = FName(EventObject.at("Name").ToString());
            }
            if (EventObject.hasKey("Time"))
            {
                Event.Time = static_cast<float>(EventObject.at("Time").ToFloat());
            }
            if (EventObject.hasKey("Duration"))
            {
                Event.Duration = static_cast<float>(EventObject.at("Duration").ToFloat());
            }
            if (EventObject.hasKey("Color"))
            {
                Event.Color = ReadColorKey(EventObject.at("Color"));
            }
            if (EventObject.hasKey("EventType"))
            {
                Event.EventType = AnimNotifyEventTypeFromString(EventObject.at("EventType").ToString());
            }
            if (EventObject.hasKey("NotifyClassName"))
            {
                Event.NotifyClassName = EventObject.at("NotifyClassName").ToString();
            }
            if (EventObject.hasKey("Payload"))
            {
                Event.Payload = EventObject.at("Payload").ToString();
            }
            if (Event.NotifyClassName.empty())
            {
                Event.NotifyClassName = GetDefaultAnimNotifyClassName(Event.EventType);
            }
            Event.EnsureStableId();
            Track.Events.push_back(Event);
        }

        return Track;
    }

    FString TrimJsonToken(FString Value)
    {
        auto IsTrimChar = [](unsigned char Ch)
        {
            return std::isspace(Ch) != 0 || Ch == ',' || Ch == '"';
        };

        while (!Value.empty() && IsTrimChar(static_cast<unsigned char>(Value.front())))
        {
            Value.erase(Value.begin());
        }

        while (!Value.empty() && IsTrimChar(static_cast<unsigned char>(Value.back())))
        {
            Value.pop_back();
        }

        return Value;
    }

    bool TryReadJsonStringValue(const FString& Line, const char* Key, FString& OutValue)
    {
        const FString KeyPattern = FString("\"") + Key + "\"";
        const size_t KeyPos = Line.find(KeyPattern);
        if (KeyPos == FString::npos)
        {
            return false;
        }

        const size_t ColonPos = Line.find(':', KeyPos + KeyPattern.size());
        if (ColonPos == FString::npos)
        {
            return false;
        }

        const size_t FirstQuotePos = Line.find('"', ColonPos + 1);
        if (FirstQuotePos == FString::npos)
        {
            return false;
        }

        const size_t SecondQuotePos = Line.find('"', FirstQuotePos + 1);
        if (SecondQuotePos == FString::npos || SecondQuotePos <= FirstQuotePos + 1)
        {
            return false;
        }

        OutValue = Line.substr(FirstQuotePos + 1, SecondQuotePos - FirstQuotePos - 1);
        return true;
    }

    bool TryReadJsonIntValue(const FString& Line, const char* Key, int32& OutValue)
    {
        const FString KeyPattern = FString("\"") + Key + "\"";
        const size_t KeyPos = Line.find(KeyPattern);
        if (KeyPos == FString::npos)
        {
            return false;
        }

        const size_t ColonPos = Line.find(':', KeyPos + KeyPattern.size());
        if (ColonPos == FString::npos)
        {
            return false;
        }

        FString Token = TrimJsonToken(Line.substr(ColonPos + 1));
        if (Token.empty())
        {
            return false;
        }

        try
        {
            OutValue = static_cast<int32>(std::stoi(Token));
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
}

UAnimSequence* FAnimSequenceAssetLoader::Load(const FString& Path) const
{
    const FString NormalizedPath = NormalizeSequencePath(Path);
    if (NormalizedPath.empty() || !IsSequenceAssetPath(NormalizedPath))
    {
        return nullptr;
    }

    std::ifstream SequenceFile(std::filesystem::path(FPaths::ToAbsolute(FPaths::ToWide(NormalizedPath))));
    if (!SequenceFile.is_open())
    {
        UE_LOG_ERROR("[AnimSequenceAssetLoader] Failed to open sequence asset: %s", NormalizedPath.c_str());
        return nullptr;
    }

    FString FileContent((std::istreambuf_iterator<char>(SequenceFile)), std::istreambuf_iterator<char>());
    json::JSON Root = json::JSON::Load(FileContent);
    if (Root.JSONType() != json::JSON::Class::Object)
    {
        UE_LOG_ERROR("[AnimSequenceAssetLoader] Invalid sequence asset json: %s", NormalizedPath.c_str());
        return nullptr;
    }

    UAnimSequence* Sequence = UObjectManager::Get().CreateObject<UAnimSequence>();
    UAnimDataModel* DataModel = new UAnimDataModel();

    const FString SequenceName = Root.hasKey("ObjectName") ? Root["ObjectName"].ToString() : FString();
    Sequence->SetFName(FName(SequenceName.empty() ? "AnimSequence" : SequenceName));
    if (Root.hasKey("SkeletonAssetPath"))
    {
        Sequence->SetSkeletonAssetPath(FPaths::ToProjectRelativePath(Root["SkeletonAssetPath"].ToString()));
    }

    DataModel->SetFName(FName(Sequence->GetName() + FString("_Data")));
    DataModel->FrameRate = FFrameRate(
        Root.hasKey("FrameRateNumerator") ? static_cast<int32>(Root["FrameRateNumerator"].ToInt()) : 30,
        Root.hasKey("FrameRateDenominator") ? static_cast<int32>(Root["FrameRateDenominator"].ToInt()) : 1);
    DataModel->NumberOfFrames = Root.hasKey("NumberOfFrames") ? static_cast<int32>(Root["NumberOfFrames"].ToInt()) : 0;
    DataModel->NumberOfKeys = Root.hasKey("NumberOfKeys") ? static_cast<int32>(Root["NumberOfKeys"].ToInt()) : 0;

    if (Root.hasKey("Tracks") && Root["Tracks"].JSONType() == json::JSON::Class::Array)
    {
        json::JSON& Tracks = Root["Tracks"];
        DataModel->BoneAnimationTracks.reserve(Tracks.length());
        for (int32 TrackIndex = 0; TrackIndex < Tracks.length(); ++TrackIndex)
        {
            json::JSON& TrackObject = Tracks.at(static_cast<unsigned>(TrackIndex));
            if (TrackObject.JSONType() != json::JSON::Class::Object)
            {
                continue;
            }

            FBoneAnimationTrack Track;
            Track.Name = FName(TrackObject.hasKey("Name") ? TrackObject["Name"].ToString() : FString());
            Track.BoneTreeIndex = TrackObject.hasKey("BoneTreeIndex")
                ? static_cast<int32>(TrackObject["BoneTreeIndex"].ToInt())
                : -1;

            if (TrackObject.hasKey("PosKeys"))
            {
                ReadVectorKeys(TrackObject["PosKeys"], Track.InternalTrackData.PosKeys);
            }
            if (TrackObject.hasKey("RotKeys"))
            {
                ReadQuatKeys(TrackObject["RotKeys"], Track.InternalTrackData.RotKeys);
            }
            if (TrackObject.hasKey("ScaleKeys"))
            {
                ReadVectorKeys(TrackObject["ScaleKeys"], Track.InternalTrackData.ScaleKeys);
            }

            DataModel->BoneAnimationTracks.push_back(Track);
        }
    }

    if (Root.hasKey("FloatCurves") && Root["FloatCurves"].JSONType() == json::JSON::Class::Array)
    {
        json::JSON& FloatCurves = Root["FloatCurves"];
        DataModel->CurveData.FloatCurves.reserve(FloatCurves.length());
        for (int32 CurveIndex = 0; CurveIndex < FloatCurves.length(); ++CurveIndex)
        {
            json::JSON& CurveObject = FloatCurves.at(static_cast<unsigned>(CurveIndex));
            if (CurveObject.JSONType() == json::JSON::Class::Object)
            {
                DataModel->CurveData.FloatCurves.push_back(ReadFloatCurve(CurveObject));
            }
        }
    }

    if (Root.hasKey("NotifyTracks") && Root["NotifyTracks"].JSONType() == json::JSON::Class::Array)
    {
        json::JSON& NotifyTracks = Root["NotifyTracks"];
        DataModel->NotifyTracks.reserve(NotifyTracks.length());
        for (int32 TrackIndex = 0; TrackIndex < NotifyTracks.length(); ++TrackIndex)
        {
            json::JSON& TrackObject = NotifyTracks.at(static_cast<unsigned>(TrackIndex));
            if (TrackObject.JSONType() == json::JSON::Class::Object)
            {
                DataModel->NotifyTracks.push_back(ReadNotifyTrack(TrackObject));
            }
        }
    }

    Sequence->DataModel = DataModel;
    return Sequence;
}

bool FAnimSequenceAssetLoader::LoadMetadata(const FString& Path, FAnimSequenceAssetMetadata& OutMetadata) const
{
    OutMetadata = {};

    const FString NormalizedPath = NormalizeSequencePath(Path);
    if (NormalizedPath.empty() || !IsSequenceAssetPath(NormalizedPath))
    {
        return false;
    }

    std::ifstream SequenceFile(FPaths::ToWide(NormalizedPath));
    if (!SequenceFile.is_open())
    {
        return false;
    }

    FString Line;
    while (std::getline(SequenceFile, Line))
    {
        FString StringValue;
        int32 IntValue = 0;

        if (TryReadJsonStringValue(Line, "ObjectName", StringValue))
        {
            OutMetadata.ObjectName = StringValue;
            continue;
        }

        if (TryReadJsonStringValue(Line, "SkeletonAssetPath", StringValue))
        {
            OutMetadata.SkeletonAssetPath = FPaths::Normalize(StringValue);
            continue;
        }

        if (TryReadJsonIntValue(Line, "FrameRateNumerator", IntValue))
        {
            OutMetadata.FrameRateNumerator = IntValue;
            continue;
        }

        if (TryReadJsonIntValue(Line, "FrameRateDenominator", IntValue))
        {
            OutMetadata.FrameRateDenominator = IntValue;
            continue;
        }

        if (TryReadJsonIntValue(Line, "NumberOfFrames", IntValue))
        {
            OutMetadata.NumberOfFrames = IntValue;
            continue;
        }

        if (TryReadJsonIntValue(Line, "NumberOfKeys", IntValue))
        {
            OutMetadata.NumberOfKeys = IntValue;
            continue;
        }

        size_t SearchPos = 0;
        while ((SearchPos = Line.find("\"BoneTreeIndex\"", SearchPos)) != FString::npos)
        {
            ++OutMetadata.BoneTrackCount;
            SearchPos += 15;
        }
    }

    return OutMetadata.IsValid();
}

bool FAnimSequenceAssetLoader::Save(const FString& Path, const UAnimSequence* Sequence) const
{
    if (!Sequence || !Sequence->DataModel)
    {
        return false;
    }

    const FString NormalizedPath = NormalizeSequencePath(Path);
    if (NormalizedPath.empty() || !IsSequenceAssetPath(NormalizedPath))
    {
        return false;
    }

    const UAnimDataModel* DataModel = Sequence->DataModel;
    json::JSON Root = json::JSON::Make(json::JSON::Class::Object);
    Root["Type"] = "UAnimSequence";
    Root["ObjectName"] = Sequence->GetName();
    Root["SkeletonAssetPath"] = FPaths::ToProjectRelativePath(Sequence->GetSkeletonAssetPath());
    Root["FrameRateNumerator"] = DataModel->FrameRate.Numerator;
    Root["FrameRateDenominator"] = DataModel->FrameRate.Denominator;
    Root["NumberOfFrames"] = DataModel->NumberOfFrames;
    Root["NumberOfKeys"] = DataModel->NumberOfKeys;

    Root["Tracks"] = json::Array();
    for (const FBoneAnimationTrack& Track : DataModel->BoneAnimationTracks)
    {
        json::JSON TrackObject = json::JSON::Make(json::JSON::Class::Object);
        TrackObject["Name"] = Track.Name.ToString();
        TrackObject["BoneTreeIndex"] = Track.BoneTreeIndex;
        WriteVectorKeys(TrackObject["PosKeys"], Track.InternalTrackData.PosKeys);
        WriteQuatKeys(TrackObject["RotKeys"], Track.InternalTrackData.RotKeys);
        WriteVectorKeys(TrackObject["ScaleKeys"], Track.InternalTrackData.ScaleKeys);
        Root["Tracks"].append(TrackObject);
    }

    Root["FloatCurves"] = json::Array();
    for (const FFloatCurve& Curve : DataModel->CurveData.FloatCurves)
    {
        json::JSON CurveObject = json::JSON::Make(json::JSON::Class::Object);
        WriteFloatCurve(CurveObject, Curve);
        Root["FloatCurves"].append(CurveObject);
    }

    Root["NotifyTracks"] = json::Array();
    for (const FAnimNotifyTrack& Track : DataModel->NotifyTracks)
    {
        json::JSON TrackObject = json::JSON::Make(json::JSON::Class::Object);
        WriteNotifyTrack(TrackObject, Track);
        Root["NotifyTracks"].append(TrackObject);
    }

    std::error_code ErrorCode;
    const std::filesystem::path FilePath(FPaths::ToAbsolute(FPaths::ToWide(NormalizedPath)));
    std::filesystem::create_directories(FilePath.parent_path(), ErrorCode);

    std::ofstream OutFile(FilePath);
    if (!OutFile.is_open())
    {
        UE_LOG_ERROR("[AnimSequenceAssetLoader] Failed to open sequence asset for writing: %s", NormalizedPath.c_str());
        return false;
    }

    OutFile << Root.dump(4);
    return true;
}

bool FAnimSequenceAssetLoader::SupportsExtension(const FString& Extension) const
{
    return Extension == ".sequence" || Extension == "sequence";
}

FString FAnimSequenceAssetLoader::GetLoaderName() const
{
    return "FAnimSequenceAssetLoader";
}
