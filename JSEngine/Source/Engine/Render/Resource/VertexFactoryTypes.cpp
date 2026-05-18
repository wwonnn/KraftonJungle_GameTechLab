#include "VertexFactoryTypes.h"
#include "Render/Scene/RenderCommand.h"
#include "Render/Mesh/VertexFactory/SkeletalVertexFactoryData.h"

void BindVertexFactoryResources(
    ID3D11DeviceContext* Context,
    EVertexFactoryType Type,
    const FRenderCommand& Cmd,
    FRenderResources* RenderResources)
{
    switch (Type)
    {
    case EVertexFactoryType::SkeletalMesh:

		if (!Cmd.VertexFactoryData)
            break;
		FSkeletalVertexFactoryData* VFData = static_cast<FSkeletalVertexFactoryData*>(Cmd.VertexFactoryData);

		if (VFData)
        {
            RenderResources->SkinningBuffer.Update(Context, VFData->SkinningMatrices->data(), VFData->SkinningMatrices->size());
            ID3D11ShaderResourceView* SkinningSRV = RenderResources->SkinningBuffer.GetSRV();
            Context->VSSetShaderResources(16, 1, &SkinningSRV);
		}
        break;
    }
}
