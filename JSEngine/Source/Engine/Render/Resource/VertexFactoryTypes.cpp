#include "VertexFactoryTypes.h"
#include "Render/Scene/RenderCommand.h"

void BindVertexFactoryResources(
    ID3D11DeviceContext* Context,
    EVertexFactoryType Type,
    const FRenderCommand& Cmd,
    FRenderResources* RenderResources)
{
    switch (Type)
    {
    case EVertexFactoryType::SkeletalMesh:
		if (Cmd.SkinningMatrices)
        {
            RenderResources->SkinningBuffer.Update(Context, Cmd.SkinningMatrices->data(), Cmd.SkinningMatrices->size());
            ID3D11ShaderResourceView* SkinningSRV = RenderResources->SkinningBuffer.GetSRV();
            Context->VSSetShaderResources(16, 1, &SkinningSRV);
		}
        break;
    }
}
