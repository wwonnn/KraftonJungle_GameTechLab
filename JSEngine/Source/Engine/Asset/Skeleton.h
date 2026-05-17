#pragma once

#include "Object/Object.h"
#include "SkeletonTypes.h"

class USkeleton : public UObject
{
public:
    DECLARE_CLASS(USkeleton, UObject)

    USkeleton() = default;
    ~USkeleton() override;

    void SetSkeletonData(FSkeletonData* InSkeletonData);

    FSkeletonData* GetSkeletonData();
    const FSkeletonData* GetSkeletonData() const;

    const FString& GetAssetPathFileName() const;

    const TArray<FSkeletonBone>& GetBones() const;
    const TArray<FSkeletonSocket>& GetSockets() const;
    const TArray<FSkeletonCurveMetaData>& GetCurveMetaData() const;

    int32 FindBoneIndex(const FString& BoneName) const;
    const FSkeletonBone* GetBoneInfo(int32 BoneIndex) const;

    const FMatrix& GetLocalBindTransform(int32 BoneIndex) const;
    const FMatrix& GetGlobalBindTransform(int32 BoneIndex) const;
    const FMatrix& GetInverseBindPose(int32 BoneIndex) const;

    const FSkeletonSocket* FindSocket(const FName& Name) const;
    bool HasSocket(const FName& Name) const;

    const FSkeletonCurveMetaData* FindCurveMetaData(const FName& Name) const;
    bool HasCurveMetaData(const FName& Name) const;

    bool HasValidSkeletonData() const;

private:
    FSkeletonData* SkeletonData = nullptr;
};
