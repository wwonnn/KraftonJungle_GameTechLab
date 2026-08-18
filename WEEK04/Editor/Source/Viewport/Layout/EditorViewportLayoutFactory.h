#pragma once

#include "EditorViewportLayout.h"
#include "EditorViewportLayoutType.h"

class FEditorViewportLayoutFactory
{
  public:
    static FEditorViewportLayout* Create(EViewportLayoutType Type);
};