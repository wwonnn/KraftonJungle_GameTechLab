#pragma once

#include "RHI/RHIBuffer.h"
#include "RHI/D3D11/D3D11Common.h"

namespace RHI::D3D11
{

// ======================== D3D11 Buffer Base ==========================

class FD3D11Buffer : public FRHIBuffer
{
public:
    FD3D11Buffer(const FBufferDesc& InDesc, TComPtr<ID3D11Buffer> InBuffer)
        : Buffer(std::move(InBuffer))
    {
        Desc = InDesc;
    }
    
    ~FD3D11Buffer() override
    {
        if (Buffer)
        {
            Buffer->Release();
            Buffer = nullptr;
        }
    }

    ID3D11Buffer* GetBuffer() const
    {
        return Buffer.Get();
    }

protected:
    TComPtr<ID3D11Buffer> Buffer;
};

// ======================== D3D11 Vertex Buffer ==========================

class FD3D11VertexBuffer : public FRHIVertexBuffer
{
public:
    FD3D11VertexBuffer(const FBufferDesc& InDesc, TComPtr<ID3D11Buffer> InBuffer)
        : Buffer(std::move(InBuffer))
    {
        Desc = InDesc;
    }
    
    ~FD3D11VertexBuffer() override
    {
        if (Buffer)
        {
            Buffer->Release();
            Buffer = nullptr;
        }
    }

    ID3D11Buffer* GetBuffer() const
    {
        return Buffer.Get();
    }

private:
    TComPtr<ID3D11Buffer> Buffer;
};

// ======================== D3D11 Index Buffer ==========================

class FD3D11IndexBuffer : public FRHIIndexBuffer
{
public:
    FD3D11IndexBuffer(const FBufferDesc& InDesc,
                      EIndexFormat InIndexFormat,
                      TComPtr<ID3D11Buffer> InBuffer)
        : Buffer(std::move(InBuffer))
    {
        Desc = InDesc;
        IndexFormat = InIndexFormat;
    }
    
    ~FD3D11IndexBuffer() override
    {
        if (Buffer)
        {
            Buffer->Release();
            Buffer = nullptr;
        }
    }

    ID3D11Buffer* GetBuffer() const
    {
        return Buffer.Get();
    }

private:
    TComPtr<ID3D11Buffer> Buffer;
};
 
// ======================== D3D11 Constant Buffer ==========================

class FD3D11ConstantBuffer : public FRHIConstantBuffer
{
public:
    FD3D11ConstantBuffer(const FBufferDesc& InDesc, TComPtr<ID3D11Buffer> InBuffer)
        : Buffer(std::move(InBuffer))
    {
        Desc = InDesc;
    }
    
    ~FD3D11ConstantBuffer() override
    {
        if (Buffer)
        {
            Buffer->Release();
            Buffer = nullptr;
        }
    }

    ID3D11Buffer* GetBuffer() const
    {
        return Buffer.Get();
    }

private:
    TComPtr<ID3D11Buffer> Buffer;
};

} // namespace RHI::D3D11
