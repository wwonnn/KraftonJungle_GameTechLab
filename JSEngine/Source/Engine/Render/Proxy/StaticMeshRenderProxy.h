#pragma once
#include "PrimitiveRenderProxy.h"

class UStaticMeshComponent;
class FStaticVertexFactoryData;

class FStaticMeshRenderProxy : public FPrimitiveRenderProxy
{
public:
    void CollectRenderCommands(const FRenderProxyContext& Context, FRenderBus& RenderBus) override;
    void Release() override;

    UStaticMeshComponent* StaticMeshComp = nullptr;
    FStaticVertexFactoryData* StaticVFData = nullptr;
};
