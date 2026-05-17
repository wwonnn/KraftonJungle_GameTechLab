#pragma once
#include "RenderProxy.h"

class FPrimitiveRenderProxy : public IRenderProxy
{
public:
    void CollectRenderCommands(const FRenderProxyContext& Context, FRenderBus& RenderBus) override;
};
