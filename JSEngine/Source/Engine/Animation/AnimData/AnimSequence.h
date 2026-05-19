#pragma once

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequenceBase.h"

class USkeleton;

class UAnimSequence : public UAnimSequenceBase
{
public:
    ~UAnimSequence() override
    {
        delete DataModel;
        DataModel = nullptr;
    }

    void SetSkeleton(USkeleton* InSkeleton)
    {
        Skeleton = InSkeleton;
    }

    USkeleton* GetSkeleton() const
    {
        return Skeleton;
    }

    void SetSkeletonAssetPath(const FString& InSkeletonAssetPath)
    {
        SkeletonAssetPath = InSkeletonAssetPath;
    }

    const FString& GetSkeletonAssetPath() const
    {
        return SkeletonAssetPath;
    }

    const TArray<FAnimNotifyTrack>& GetNotifyTracks() const
    {
        static const TArray<FAnimNotifyTrack> EmptyTracks;
        return DataModel ? DataModel->NotifyTracks : EmptyTracks;
    }

    TArray<FAnimNotifyTrack>& GetMutableNotifyTracks()
    {
        static TArray<FAnimNotifyTrack> EmptyTracks;
        return DataModel ? DataModel->NotifyTracks : EmptyTracks;
    }

    UAnimDataModel* DataModel = nullptr;
    FString SkeletonAssetPath;
    USkeleton* Skeleton = nullptr;
};
