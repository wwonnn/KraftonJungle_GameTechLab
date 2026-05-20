#pragma once

#include "Core/CoreMinimal.h"

struct FAclLibraryInfo
{
    int32 Major = 0;
    int32 Minor = 0;
    int32 Patch = 0;
};

class FAclCompression
{
public:
    static FAclLibraryInfo GetLibraryInfo();
};
