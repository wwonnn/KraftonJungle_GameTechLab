#pragma once
#include "PrimitiveRenderProxy.h"

class UStaticMeshComponent;

class FStaticMeshRenderProxy : public FPrimitiveRenderProxy
{
public:
    void CollectRenderCommands(const FRenderProxyContext& Context, FRenderBus& RenderBus) override;

    UStaticMeshComponent* StaticMeshComp = nullptr;
};
