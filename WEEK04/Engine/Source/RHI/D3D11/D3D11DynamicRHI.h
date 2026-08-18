#pragma once

#include "RHI/DynamicRHI.h"
#include "RHI/D3D11/D3D11Texture.h"
#include "RHI/D3D11/D3D11Buffer.h"
#include "Core/EngineAPI.h"

namespace RHI::D3D11
{

    // ======================== D3D11 Dynamic RHI ==========================

    class ENGINE_API FD3D11DynamicRHI : public FDynamicRHI
    {
      public:
        FD3D11DynamicRHI(TComPtr<ID3D11Device>        InDevice,
                         TComPtr<ID3D11DeviceContext> InDeviceContext)
            : Device(std::move(InDevice)), DeviceContext(std::move(InDeviceContext))
        {
        }

        virtual std::shared_ptr<FRHITexture2D>
        CreateTexture2D(const FTextureDesc& Desc, const void* InitialData = nullptr,
                        uint32 InitialDataPitch = 0) override;

        virtual std::shared_ptr<FRHIVertexBuffer>
        CreateVertexBuffer(const FBufferDesc& Desc, const void* InitialData = nullptr) override;

        virtual std::shared_ptr<FRHIIndexBuffer>
        CreateIndexBuffer(const FBufferDesc& Desc, EIndexFormat IndexFormat,
                          const void* InitialData = nullptr) override;

        virtual std::shared_ptr<FRHIConstantBuffer>
        CreateConstantBuffer(const FBufferDesc& Desc, const void* InitialData = nullptr) override;

        virtual bool UpdateBuffer(FRHIBuffer& Buffer, const void* Data, uint32 ByteSize) override;

        ID3D11Device* GetDevice() const { return Device.Get(); }

        ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext.Get(); }

      private:
        TComPtr<ID3D11Buffer> CreateBufferInternal(const FBufferDesc& Desc,
                                                   const void*        InitialData);

      private:
        TComPtr<ID3D11Device>        Device;
        TComPtr<ID3D11DeviceContext> DeviceContext;
    };

} // namespace RHI::D3D11
