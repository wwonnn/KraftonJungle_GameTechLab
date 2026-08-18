#pragma once

#include "RHI/RHIResource.h"
#include "RHI/RHITypes.h"

namespace RHI
{

    // ======================== Buffer Resource ==========================

    class ENGINE_API FRHIBuffer : public FRHIResource
    {
      public:
        const FBufferDesc& GetDesc() const { return Desc; }

      protected:
        FBufferDesc Desc;
    };

    // ======================== Vertex Buffer ==========================

    class ENGINE_API FRHIVertexBuffer : public FRHIBuffer
    {

    };

    // ======================== Index Buffer ==========================

    class ENGINE_API FRHIIndexBuffer : public FRHIBuffer
    {
      public:
        EIndexFormat GetIndexFormat() const { return IndexFormat; }

      protected:
        EIndexFormat IndexFormat = EIndexFormat::UInt32;
    };

    // ======================== Constant Buffer ==========================

    class ENGINE_API FRHIConstantBuffer : public FRHIBuffer
    {

    };

} // namespace RHI
