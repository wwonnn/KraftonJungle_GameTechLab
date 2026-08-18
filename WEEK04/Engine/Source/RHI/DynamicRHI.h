#pragma once

#include <memory>

#include "RHI/RHITexture.h"
#include "RHI/RHIBuffer.h"

namespace RHI
{

    // ======================== Dynamic RHI ==========================

    class FDynamicRHI
    {
      public:
        virtual ~FDynamicRHI() = default;

        virtual std::shared_ptr<FRHITexture2D> CreateTexture2D(const FTextureDesc& Desc,
                                                               const void* InitialData = nullptr,
                                                               uint32 InitialDataPitch = 0) = 0;

        virtual std::shared_ptr<FRHIVertexBuffer>
        CreateVertexBuffer(const FBufferDesc& Desc, const void* InitialData = nullptr) = 0;

        virtual std::shared_ptr<FRHIIndexBuffer>
        CreateIndexBuffer(const FBufferDesc& Desc, EIndexFormat IndexFormat,
                          const void* InitialData = nullptr) = 0;

        virtual std::shared_ptr<FRHIConstantBuffer>
        CreateConstantBuffer(const FBufferDesc& Desc, const void* InitialData = nullptr) = 0;

        virtual bool UpdateBuffer(FRHIBuffer& Buffer, const void* Data, uint32 ByteSize) = 0;
    };

} // namespace RHI
