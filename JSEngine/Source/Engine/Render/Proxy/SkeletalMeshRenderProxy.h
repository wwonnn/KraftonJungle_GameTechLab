#pragma once
#include "PrimitiveRenderProxy.h"

class USkeletalMeshComponent;
class FSkeletalVertexFactoryData;

class FSkeletalMeshRenderProxy : public FPrimitiveRenderProxy
{
public:
    void CollectRenderCommands(const FRenderProxyContext& Context, FRenderBus& RenderBus) override;
    void Release() override;

	USkeletalMeshComponent* SkeletalMeshComp = nullptr;
    FSkeletalVertexFactoryData* SkelVFData = nullptr;
};
