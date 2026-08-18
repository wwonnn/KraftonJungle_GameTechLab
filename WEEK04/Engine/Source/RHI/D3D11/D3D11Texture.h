#pragma once

#include "RHI/RHITexture.h"
#include "RHI/D3D11/D3D11Common.h"

namespace RHI::D3D11
{

    // ======================== D3D11 Texture 2D ==========================

    class FD3D11Texture2D : public FRHITexture2D
    {
      public:
        FD3D11Texture2D(const FTextureDesc& InDesc, TComPtr<ID3D11Texture2D> InTexture,
                        TComPtr<ID3D11ShaderResourceView> InSRV = nullptr)
            : Texture(std::move(InTexture)), SRV(std::move(InSRV))
        {
            Desc = InDesc;
        }

        ID3D11Texture2D* GetTexture() const { return Texture.Get(); }

        ID3D11ShaderResourceView* GetSRV() const { return SRV.Get(); }

      private:
        TComPtr<ID3D11Texture2D>          Texture;
        TComPtr<ID3D11ShaderResourceView> SRV;
    };

} // namespace RHI::D3D11
