#pragma once
#pragma once

#include "Engine/ViewPort/ViewportController.h"

struct FEditorContext;
class FViewportSelectionController;

class FEditorGlobalController : public Engine::Viewport::IViewportController
{
public:
    FEditorGlobalController() = default;
    ~FEditorGlobalController() override = default;

    void SetEditorContext(FEditorContext* InContext) { Context = InContext; }
    void SetSelectionController(FViewportSelectionController* InSelectionController)
    {
        SelectionController = InSelectionController;
    }

    bool CanDeleteSelectedActors() const;
    bool DeleteSelectedActors();

    void NewScene();
    void OpenScene();
    void SaveScene();
    void SaveSceneAs();
    void RequestAboutPopup();

private:
    FEditorContext* Context = nullptr;
    FViewportSelectionController* SelectionController = nullptr;
};
