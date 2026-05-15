#pragma once

#include "Object/Object.h"

struct FAnimKeyVector
{
    float Time;
    FVector Value;
};

struct FAnimKeyQuat
{
    float Time;
    FQuat Value;
};

struct FBoneAnimationTrack
{
    int32 BoneIndex;
    FName BoneName;

    TArray<FAnimKeyVector> PosKeys;
    TArray<FAnimKeyQuat> RotKeys;
    TArray<FAnimKeyVector> ScaleKeys;
};

class UAnimDataModel
{
public:
    float SequenceLength = 0.0f;
    float SampleRate = 30.0f;
    int32 NumFrames = 0;

    TArray<FBoneAnimationTrack> BoneTracks;
};

class UAnimSequence : public UObject
{
public:
    DECLARE_CLASS(UAnimSequence, UObject)
public:
    UAnimDataModel* DataModel = nullptr;
};
