#pragma once

#include "Core/CoreMinimal.h"

namespace RHI
{

    // ======================== Pixel Format ==========================

    enum class EPixelFormat : uint8
    {
        Unknown = 0,
        R8,
        RG8,
        RGB8,
        RGBA8,
        BGRA8,
        R16F,
        RG16F,
        RGBA16F,
        R32F,
        RG32F,
        RGBA32F,
        D24S8,
        D32F,
    };

    // ======================== Buffer Usage ==========================

    enum class EBufferUsage : uint8
    {
        Default = 0,
        Dynamic,
        Immutable,
        Staging,
    };

    // ======================== CPU Access Flags ==========================

    enum class ECPUAccessFlags : uint8
    {
        None = 0,
        Read = 1 << 0,
        Write = 1 << 1,
    };

    inline ECPUAccessFlags operator|(ECPUAccessFlags A, ECPUAccessFlags B)
    {
        return static_cast<ECPUAccessFlags>(static_cast<uint8>(A) | static_cast<uint8>(B));
    }

    inline ECPUAccessFlags operator&(ECPUAccessFlags A, ECPUAccessFlags B)
    {
        return static_cast<ECPUAccessFlags>(static_cast<uint8>(A) & static_cast<uint8>(B));
    }

    inline bool HasAnyFlags(ECPUAccessFlags Value, ECPUAccessFlags Flags)
    {
        return static_cast<uint8>(Value & Flags) != 0;
    }

    // ======================== Texture Bind Flags ==========================

    enum class ETextureBindFlags : uint8
    {
        None = 0,
        ShaderResource = 1 << 0,
        RenderTarget = 1 << 1,
        DepthStencil = 1 << 2,
    };

    inline ETextureBindFlags operator|(ETextureBindFlags A, ETextureBindFlags B)
    {
        return static_cast<ETextureBindFlags>(static_cast<uint8>(A) | static_cast<uint8>(B));
    }

    inline ETextureBindFlags operator&(ETextureBindFlags A, ETextureBindFlags B)
    {
        return static_cast<ETextureBindFlags>(static_cast<uint8>(A) & static_cast<uint8>(B));
    }

    inline bool HasAnyFlags(ETextureBindFlags Value, ETextureBindFlags Flags)
    {
        return static_cast<uint8>(Value & Flags) != 0;
    }

    // ======================== Buffer Bind Flags ==========================

    enum class EBufferBindFlags : uint8
    {
        None = 0,
        VertexBuffer = 1 << 0,
        IndexBuffer = 1 << 1,
        ConstantBuffer = 1 << 2,
        ShaderResource = 1 << 3,
    };

    inline EBufferBindFlags operator|(EBufferBindFlags A, EBufferBindFlags B)
    {
        return static_cast<EBufferBindFlags>(static_cast<uint8>(A) | static_cast<uint8>(B));
    }

    inline EBufferBindFlags operator&(EBufferBindFlags A, EBufferBindFlags B)
    {
        return static_cast<EBufferBindFlags>(static_cast<uint8>(A) & static_cast<uint8>(B));
    }

    inline bool HasAnyFlags(EBufferBindFlags Value, EBufferBindFlags Flags)
    {
        return static_cast<uint8>(Value & Flags) != 0;
    }

    // ======================== Index Format ==========================

    enum class EIndexFormat : uint8
    {
        UInt16 = 0,
        UInt32,
    };

    // ======================== Texture Desc ==========================

    struct FTextureDesc
    {
        uint32            Width = 0;
        uint32            Height = 0;
        uint32            MipLevels = 1;
        uint32            ArraySize = 1;
        EPixelFormat      Format = EPixelFormat::Unknown;
        ETextureBindFlags BindFlags = ETextureBindFlags::ShaderResource;
        EBufferUsage      Usage = EBufferUsage::Default;
        ECPUAccessFlags   CPUAccessFlags = ECPUAccessFlags::None;
    };

    // ======================== Buffer Desc ==========================

    struct FBufferDesc
    {
        uint32           ByteWidth = 0;
        uint32           Stride = 0;
        EBufferBindFlags BindFlags = EBufferBindFlags::None;
        EBufferUsage     Usage = EBufferUsage::Default;
        ECPUAccessFlags  CPUAccessFlags = ECPUAccessFlags::None;
    };

} // namespace RHI
