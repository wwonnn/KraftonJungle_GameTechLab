#pragma once
#include "PrimitiveRenderProxy.h"

class USkeletalMeshComponent;

class FSkeletalMeshRenderProxy : public FPrimitiveRenderProxy
{
public:
    void CollectRenderCommands(const FRenderProxyContext& Context, FRenderBus& RenderBus) override;

	USkeletalMeshComponent* SkeletalMeshComp = nullptr;
};
