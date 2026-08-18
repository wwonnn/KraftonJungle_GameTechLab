#pragma once

#include "Core/CoreMinimal.h"

namespace RHI
{

    // ======================== Base Resource ==========================

    class ENGINE_API FRHIResource
    {
      public:
        virtual ~FRHIResource() = default;
    };

} // namespace RHI
