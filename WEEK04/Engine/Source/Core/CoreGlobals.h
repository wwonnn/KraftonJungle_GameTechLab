#pragma once

#include "Core/EngineAPI.h"
#include "Core/Logging/LogOutputDevice.h"

class ILogOutputDevice;
extern ENGINE_API ILogOutputDevice* GLog;
extern ENGINE_API ELogLevel         GMinimumLogLevel;
