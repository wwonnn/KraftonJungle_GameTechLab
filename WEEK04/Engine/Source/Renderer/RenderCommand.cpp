#include "RenderCommand.h"
#include "Engine/Asset/Material.h"
#include "Primitive/FMeshData.h"

void FRenderCommand::SetDefaultStates()
{
    RasterizerOption = FRasterizerStateOption();
    DepthStencilOption = FDepthStencilStateOption();
    BlendOption = FBlendStateOption();
}

void FRenderCommand::SetStates(const UMaterial* InMaterial, EMeshTopology InTopology)
{
    // FMaterial no longer carries RenderState options (transitioning to UMaterial).
    // States should be set explicitly on FRenderCommand by the component.
    Topology = InTopology;
}

uint64 FRenderCommand::MakeSortKey(const UMaterial* InMaterial, const FMeshData* InMeshData)
{
    // Material 체계 교체로 인해 
    // uint32 MatId = InMaterial ? InMaterial->GetId() : 0;
    uint32 MatId = 0;
    uint32 MeshId = InMeshData ? InMeshData->GetSortId() : 0;
    return (static_cast<uint64>(MatId) << 32) | MeshId;
}