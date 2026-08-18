#include "RHI/D3D11/D3D11Common.h"

namespace RHI::D3D11
{

// ======================== Pixel Format ==========================

DXGI_FORMAT GetDXGIFormat(EPixelFormat Format)
{
    switch (Format)
    {
    case EPixelFormat::R8: return DXGI_FORMAT_R8_UNORM;
    case EPixelFormat::RG8: return DXGI_FORMAT_R8G8_UNORM;
    case EPixelFormat::RGB8: return DXGI_FORMAT_UNKNOWN;
    case EPixelFormat::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case EPixelFormat::BGRA8: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case EPixelFormat::R16F: return DXGI_FORMAT_R16_FLOAT;
    case EPixelFormat::RG16F: return DXGI_FORMAT_R16G16_FLOAT;
    case EPixelFormat::RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case EPixelFormat::R32F: return DXGI_FORMAT_R32_FLOAT;
    case EPixelFormat::RG32F: return DXGI_FORMAT_R32G32_FLOAT;
    case EPixelFormat::RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case EPixelFormat::D24S8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case EPixelFormat::D32F: return DXGI_FORMAT_D32_FLOAT;
    default: return DXGI_FORMAT_UNKNOWN;
    }
}

// ======================== Usage ==========================

D3D11_USAGE GetD3D11Usage(EBufferUsage Usage)
{
    switch (Usage)
    {
    case EBufferUsage::Default: return D3D11_USAGE_DEFAULT;
    case EBufferUsage::Dynamic: return D3D11_USAGE_DYNAMIC;
    case EBufferUsage::Immutable: return D3D11_USAGE_IMMUTABLE;
    case EBufferUsage::Staging: return D3D11_USAGE_STAGING;
    default: return D3D11_USAGE_DEFAULT;
    }
}

// ======================== CPU Access ==========================

UINT GetD3D11CPUAccessFlags(ECPUAccessFlags Flags)
{
    UINT Result = 0;
    if (HasAnyFlags(Flags, ECPUAccessFlags::Read))
    {
        Result |= D3D11_CPU_ACCESS_READ;
    }
    if (HasAnyFlags(Flags, ECPUAccessFlags::Write))
    {
        Result |= D3D11_CPU_ACCESS_WRITE;
    }
    return Result;
}

// ======================== Bind Flags ==========================

UINT GetD3D11BindFlags(ETextureBindFlags Flags)
{
    UINT Result = 0;
    if (HasAnyFlags(Flags, ETextureBindFlags::ShaderResource))
    {
        Result |= D3D11_BIND_SHADER_RESOURCE;
    }
    if (HasAnyFlags(Flags, ETextureBindFlags::RenderTarget))
    {
        Result |= D3D11_BIND_RENDER_TARGET;
    }
    if (HasAnyFlags(Flags, ETextureBindFlags::DepthStencil))
    {
        Result |= D3D11_BIND_DEPTH_STENCIL;
    }
    return Result;
}

UINT GetD3D11BindFlags(EBufferBindFlags Flags)
{
    UINT Result = 0;
    if (HasAnyFlags(Flags, EBufferBindFlags::VertexBuffer))
    {
        Result |= D3D11_BIND_VERTEX_BUFFER;
    }
    if (HasAnyFlags(Flags, EBufferBindFlags::IndexBuffer))
    {
        Result |= D3D11_BIND_INDEX_BUFFER;
    }
    if (HasAnyFlags(Flags, EBufferBindFlags::ConstantBuffer))
    {
        Result |= D3D11_BIND_CONSTANT_BUFFER;
    }
    if (HasAnyFlags(Flags, EBufferBindFlags::ShaderResource))
    {
        Result |= D3D11_BIND_SHADER_RESOURCE;
    }
    return Result;
}

// ======================== Index Format ==========================

DXGI_FORMAT GetD3D11IndexFormat(EIndexFormat Format)
{
    switch (Format)
    {
    case EIndexFormat::UInt16: return DXGI_FORMAT_R16_UINT;
    case EIndexFormat::UInt32: return DXGI_FORMAT_R32_UINT;
    default: return DXGI_FORMAT_R32_UINT;
    }
}

} // namespace RHI::D3D11
