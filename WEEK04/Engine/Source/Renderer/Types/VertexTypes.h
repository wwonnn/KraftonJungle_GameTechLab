#pragma once

#include "Core/Math/Vector.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Color.h"

struct ENGINE_API FPrimitiveVertex
{
    FVector  Position;
    FVector  Normal;
    FColor   Color;
    FVector2 UV;
};