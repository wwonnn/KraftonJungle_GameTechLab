#pragma once

#include "Editor/UI/EditorWidget.h"

class UAnimInstanceAsset;

class FEditorAnimInstanceEditorWidget : public FEditorWidget
{
public:
    void Render(float DeltaTime) override;

    void OpenAnimInstanceAsset(const FString& AnimInstancePath);
    bool IsVisible() const { return bVisible; }

private:
    void DrawToolbar();
    void DrawGraph();
    void DrawDetails();
    void AddState();
    void AddTransition();
    bool SaveAsset();
    bool ReloadAsset();
    void MarkDirty() { bDirty = true; }

private:
    FString CurrentPath;
    UAnimInstanceAsset* CurrentAsset = nullptr;
    int32 SelectedStateIndex = -1;
    int32 SelectedTransitionIndex = -1;
    bool bVisible = false;
    bool bDirty = false;
};
