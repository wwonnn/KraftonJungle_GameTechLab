#pragma once

#include "RHI/RHIResource.h"
#include "RHI/RHITypes.h"

namespace RHI
{

    // ======================== Texture Resource ==========================

    class FRHITexture : public FRHIResource
    {
      public:
        virtual ~FRHITexture() = default;

        const FTextureDesc& GetDesc() const { return Desc; }

      protected:
        FTextureDesc Desc;
    };

    // ======================== 2D Texture ==========================

    class FRHITexture2D : public FRHITexture
    {
      public:
        virtual ~FRHITexture2D() = default;
    };

} // namespace RHI
