#pragma once

#include "Core/Containers/String.h"
#include "Core/Logging/LogOutputDevice.h"

struct FEditorLogEntry
{
    ELogLevel Level = ELogLevel::Info;
    FString   Message;
};
