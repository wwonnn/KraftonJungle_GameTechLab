#pragma once

#include "Core/EngineAPI.h"
#include "Core/HAL/PlatformTypes.h"

enum class ELogLevel : uint8
{
    Verbose = 0,
    Debug,
    Info,
    Warning,
    Error
};

ENGINE_API const char* GetLogLevelLabel(ELogLevel Level);
ENGINE_API ELogLevel   GetGlobalLogLevel();
ENGINE_API void        SetGlobalLogLevel(ELogLevel Level);
ENGINE_API bool        ShouldLog(ELogLevel Level);

class ENGINE_API ILogOutputDevice
{
  public:
    virtual ~ILogOutputDevice() = default;
    virtual void Log(ELogLevel Level, const char* Message) = 0;
};
