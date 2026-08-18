#include "RHI/D3D11/D3D11DynamicRHI.h"

namespace RHI::D3D11
{

    // ======================== Texture ==========================

    std::shared_ptr<FRHITexture2D> FD3D11DynamicRHI::CreateTexture2D(const FTextureDesc& Desc,
                                                                     const void* InitialData,
                                                                     uint32      InitialDataPitch)
    {
        if (Device == nullptr)
        {
            return nullptr;
        }

        D3D11_TEXTURE2D_DESC NativeDesc = {};
        NativeDesc.Width = Desc.Width;
        NativeDesc.Height = Desc.Height;
        NativeDesc.MipLevels = Desc.MipLevels;
        NativeDesc.ArraySize = Desc.ArraySize;
        NativeDesc.Format = GetDXGIFormat(Desc.Format);
        NativeDesc.SampleDesc.Count = 1;
        NativeDesc.SampleDesc.Quality = 0;
        NativeDesc.Usage = GetD3D11Usage(Desc.Usage);
        NativeDesc.BindFlags = GetD3D11BindFlags(Desc.BindFlags);
        NativeDesc.CPUAccessFlags = GetD3D11CPUAccessFlags(Desc.CPUAccessFlags);
        NativeDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA  InitialSubresource = {};
        D3D11_SUBRESOURCE_DATA* InitialSubresourcePtr = nullptr;
        if (InitialData != nullptr)
        {
            InitialSubresource.pSysMem = InitialData;
            InitialSubresource.SysMemPitch = InitialDataPitch;
            InitialSubresource.SysMemSlicePitch = 0;
            InitialSubresourcePtr = &InitialSubresource;
        }

        TComPtr<ID3D11Texture2D> Texture;
        HRESULT Hr = Device->CreateTexture2D(&NativeDesc, InitialSubresourcePtr, &Texture);
        if (FAILED(Hr) || Texture == nullptr)
        {
            return nullptr;
        }

        TComPtr<ID3D11ShaderResourceView> SRV;
        if (HasAnyFlags(Desc.BindFlags, ETextureBindFlags::ShaderResource))
        {
            Hr = Device->CreateShaderResourceView(Texture.Get(), nullptr, &SRV);
            if (FAILED(Hr))
            {
                return nullptr;
            }
        }

        return std::make_shared<FD3D11Texture2D>(Desc, std::move(Texture), std::move(SRV));
    }

    // ======================== Vertex Buffer ==========================

    std::shared_ptr<FRHIVertexBuffer> FD3D11DynamicRHI::CreateVertexBuffer(const FBufferDesc& Desc,
                                                                           const void* InitialData)
    {
        TComPtr<ID3D11Buffer> Buffer = CreateBufferInternal(Desc, InitialData);
        if (Buffer == nullptr)
        {
            return nullptr;
        }

        return std::make_shared<FD3D11VertexBuffer>(Desc, std::move(Buffer));
    }

    // ======================== Index Buffer ==========================

    std::shared_ptr<FRHIIndexBuffer> FD3D11DynamicRHI::CreateIndexBuffer(const FBufferDesc& Desc,
                                                                         EIndexFormat IndexFormat,
                                                                         const void*  InitialData)
    {
        TComPtr<ID3D11Buffer> Buffer = CreateBufferInternal(Desc, InitialData);
        if (Buffer == nullptr)
        {
            return nullptr;
        }

        return std::make_shared<FD3D11IndexBuffer>(Desc, IndexFormat, std::move(Buffer));
    }

    // ======================== Constant Buffer ==========================

    std::shared_ptr<FRHIConstantBuffer>
    FD3D11DynamicRHI::CreateConstantBuffer(const FBufferDesc& Desc, const void* InitialData)
    {
        TComPtr<ID3D11Buffer> Buffer = CreateBufferInternal(Desc, InitialData);
        if (Buffer == nullptr)
        {
            return nullptr;
        }

        return std::make_shared<FD3D11ConstantBuffer>(Desc, std::move(Buffer));
    }

    // ======================== Update Buffer ==========================

    bool FD3D11DynamicRHI::UpdateBuffer(FRHIBuffer& Buffer, const void* Data, uint32 ByteSize)
    {
        if (DeviceContext == nullptr || Data == nullptr || ByteSize == 0)
        {
            return false;
        }

        ID3D11Buffer* NativeBuffer = nullptr;

        if (auto* VB = dynamic_cast<FD3D11VertexBuffer*>(&Buffer))
        {
            NativeBuffer = VB->GetBuffer();
        }
        else if (auto* IB = dynamic_cast<FD3D11IndexBuffer*>(&Buffer))
        {
            NativeBuffer = IB->GetBuffer();
        }
        else if (auto* CB = dynamic_cast<FD3D11ConstantBuffer*>(&Buffer))
        {
            NativeBuffer = CB->GetBuffer();
        }

        if (NativeBuffer == nullptr)
        {
            return false;
        }

        const FBufferDesc& Desc = Buffer.GetDesc();
        if (Desc.Usage == EBufferUsage::Dynamic)
        {
            D3D11_MAPPED_SUBRESOURCE Mapped = {};
            HRESULT Hr = DeviceContext->Map(NativeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
            if (FAILED(Hr))
            {
                return false;
            }

            memcpy(Mapped.pData, Data, ByteSize);
            DeviceContext->Unmap(NativeBuffer, 0);
            return true;
        }

        DeviceContext->UpdateSubresource(NativeBuffer, 0, nullptr, Data, 0, 0);
        return true;
    }

    // ======================== Internal Buffer Create ==========================

    TComPtr<ID3D11Buffer> FD3D11DynamicRHI::CreateBufferInternal(const FBufferDesc& Desc,
                                                                 const void*        InitialData)
    {
        if (Device == nullptr || Desc.ByteWidth == 0)
        {
            return nullptr;
        }

        D3D11_BUFFER_DESC NativeDesc = {};
        NativeDesc.ByteWidth = Desc.ByteWidth;
        NativeDesc.Usage = GetD3D11Usage(Desc.Usage);
        NativeDesc.BindFlags = GetD3D11BindFlags(Desc.BindFlags);
        NativeDesc.CPUAccessFlags = GetD3D11CPUAccessFlags(Desc.CPUAccessFlags);
        NativeDesc.MiscFlags = 0;
        NativeDesc.StructureByteStride = Desc.Stride;

        D3D11_SUBRESOURCE_DATA  InitialSubresource = {};
        D3D11_SUBRESOURCE_DATA* InitialSubresourcePtr = nullptr;
        if (InitialData != nullptr)
        {
            InitialSubresource.pSysMem = InitialData;
            InitialSubresource.SysMemPitch = 0;
            InitialSubresource.SysMemSlicePitch = 0;
            InitialSubresourcePtr = &InitialSubresource;
        }

        TComPtr<ID3D11Buffer> Buffer;
        HRESULT Hr = Device->CreateBuffer(&NativeDesc, InitialSubresourcePtr, &Buffer);
        if (FAILED(Hr))
        {
            return nullptr;
        }

        return Buffer;
    }

} // namespace RHI::D3D11
