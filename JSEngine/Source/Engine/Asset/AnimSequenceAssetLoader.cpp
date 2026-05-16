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
        return FPaths::Normalize(Path);
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
}

UAnimSequence* FAnimSequenceAssetLoader::Load(const FString& Path) const
{
    const FString NormalizedPath = NormalizeSequencePath(Path);
    if (NormalizedPath.empty() || !IsSequenceAssetPath(NormalizedPath))
    {
        return nullptr;
    }

    std::ifstream SequenceFile(FPaths::ToWide(NormalizedPath));
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

    Sequence->DataModel = DataModel;
    return Sequence;
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

    std::error_code ErrorCode;
    const std::filesystem::path FilePath(FPaths::ToWide(NormalizedPath));
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
