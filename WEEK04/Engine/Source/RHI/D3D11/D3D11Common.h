#pragma once

#include <wrl/client.h>
#include <d3d11.h>

#include "Core/CoreMinimal.h"
#include "RHI/RHITypes.h"

template <typename T> using TComPtr = Microsoft::WRL::ComPtr<T>;

namespace RHI::D3D11
{

    // ======================== Helpers ==========================

    DXGI_FORMAT GetDXGIFormat(EPixelFormat Format);
    D3D11_USAGE GetD3D11Usage(EBufferUsage Usage);
    UINT        GetD3D11CPUAccessFlags(ECPUAccessFlags Flags);
    UINT        GetD3D11BindFlags(ETextureBindFlags Flags);
    UINT        GetD3D11BindFlags(EBufferBindFlags Flags);
    DXGI_FORMAT GetD3D11IndexFormat(EIndexFormat Format);

} // namespace RHI::D3D11
