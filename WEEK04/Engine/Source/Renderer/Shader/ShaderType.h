#pragma once
#include "Core/Math/Matrix.h"

// b0: 프레임당 1회 업데이트 (카메라)
struct FFrameConstantBuffer
{
    FMatrix View;
    FMatrix Projection;
};

// b1: 오브젝트당 업데이트
struct FObjectConstantBuffer
{
    FMatrix World;
    uint32 ObjectId;
    FVector2 UVOffset;
    uint32 Padding;
    FVector4 MultiplyColor; // Default (1, 1, 1, 1)
    FVector4 AdditiveColor; // Default (0, 0, 0, 0)
};