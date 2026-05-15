#pragma once

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequenceBase.h"

class UAnimSequence : public UAnimSequenceBase
{
public:
    ~UAnimSequence() override
    {
        delete DataModel;
        DataModel = nullptr;
    }

    UAnimDataModel* DataModel = nullptr;
};
