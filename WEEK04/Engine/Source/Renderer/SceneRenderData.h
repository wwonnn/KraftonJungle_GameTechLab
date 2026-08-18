#pragma once

#include "Core/Containers/Array.h"
#include "Renderer/Types/RenderItem.h"
#include "Renderer/Types/ViewMode.h"
#include "RenderCommand.h"

class FSceneView;

struct FSceneRenderData
{
    const FSceneView* SceneView = nullptr;
    EViewModeIndex    ViewMode = EViewModeIndex::VMI_Lit;
    bool              bUseInstancing = true;

    TArray<FRenderCommand> RenderCommands;
};