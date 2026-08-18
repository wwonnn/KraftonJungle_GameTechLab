#pragma once

#include "EditorViewportClient.h"
#include "Renderer/SceneView.h"

class FViewport
{
  public:
    FEditorViewportClient* const& GetViewportClient() const { return ViewportClient; }
    FSceneView* const&            GetSceneView() const { return SceneView; }

    void SetViewportClient(FEditorViewportClient* NewViewportClient)
    {
        ViewportClient = NewViewportClient;
    }
    void SetSceneView(FSceneView* NewSceneView) { SceneView = NewSceneView; }

    bool IsValid() { return bValid; }
    void SetValid(bool Valid) { bValid = Valid; }
    void RemoveViewportClient() { ViewportClient = nullptr; }

    void Release()
    {
        delete ViewportClient;
        ViewportClient = nullptr;

        delete SceneView;
        SceneView = nullptr;
    }

  private:
    FEditorViewportClient* ViewportClient = nullptr;
    FSceneView*            SceneView = nullptr;
    bool                   bValid = true;
};