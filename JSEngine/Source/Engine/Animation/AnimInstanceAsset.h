#pragma once

#include "Animation/AnimationStateMachine.h"
#include "Math/Vector2.h"
#include "Object/Object.h"

struct FAnimInstanceParameterAssetData
{
    FName Name;
    EAnimStateParameterType Type = EAnimStateParameterType::Float;
    bool BoolDefault = false;
    float FloatDefault = 0.0f;
    int32 IntDefault = 0;
};

struct FAnimInstanceStateAssetData
{
    FName Name;
    FString AnimSequencePath;
    bool bLoop = true;
    float PlayRate = 1.0f;
    FVector2 EditorNodePosition = FVector2(100.0f, 100.0f);
};

struct FAnimInstanceTransitionAssetData
{
    FName FromState;
    FName ToState;
    FName ParameterName;
    EAnimTransitionConditionType ConditionType = EAnimTransitionConditionType::BoolEquals;
    bool ExpectedBool = false;
    float CompareFloat = 0.0f;
    int32 ExpectedInt = 0;
    float BlendSpeed = 5.0f;
    int32 Priority = 0;
};

class UAnimInstanceAsset : public UObject
{
public:
    DECLARE_CLASS(UAnimInstanceAsset, UObject)

public:
    void SetAssetPath(const FString& InPath) { AssetPath = InPath; }
    const FString& GetAssetPath() const { return AssetPath; }

    FName EntryState;
    TArray<FAnimInstanceParameterAssetData> Parameters;
    TArray<FAnimInstanceStateAssetData> States;
    TArray<FAnimInstanceTransitionAssetData> Transitions;

private:
    FString AssetPath;
};
