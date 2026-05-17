#pragma once
#include "PrimitiveRenderProxy.h"

class FMeshBuffer;
class UStaticMesh;
class UStaticMeshComponent;

class FStaticMeshRenderProxy : public FPrimitiveRenderProxy
{
public:
    void CollectRenderCommands(const FRenderProxyContext& Context, FRenderBus& RenderBus) override;

	int32 ValidLODCount = 0;
    FAABB Bounds;
    UStaticMesh* StaticMeshAsset = nullptr;
    UStaticMeshComponent* StaticMeshComp = nullptr;
};
