#pragma once

#include <vector>
#include "Core/CoreMinimal.h"
#include "Core/Containers/Array.h"
#include "Engine/Asset/AssetObjectManager.h"

namespace RHI
{
    class FDynamicRHI;
}

class UObject;
class FD3D11RHI;
class FRendererModule;
class FWorld;
class FEditor;
class AActor;
class FEditorContentIndex;

struct FEditorContext
{
    FEditor*             Editor = nullptr;
    FWorld*              World = nullptr;
    FD3D11RHI*           RHI = nullptr;
    FRendererModule* Renderer = nullptr;
    RHI::FDynamicRHI*    DynamicRHI = nullptr;
    FAssetObjectManager* AssetObjectManager = nullptr;
    TArray<FString>      StartupPreloadAssetPaths;
    FEditorContentIndex* ContentIndex = nullptr;

    TArray<AActor*> SelectedActors;
    UObject*        SelectedObject = nullptr;

    float CurrentFPS = 0.0f;
    float DeltaTime = 0.0f;
    float WindowWidth = 0.0f;
    float WindowHeight = 0.0f;
    float ContentBrowserLeftPaneWidth = 250.0f;
};
