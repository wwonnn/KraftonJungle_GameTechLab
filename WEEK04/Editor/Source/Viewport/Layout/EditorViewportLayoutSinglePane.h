#pragma once

#include "EditorViewportLayout.h"

class FEditorViewportLayoutSinglePane : public FEditorViewportLayout
{
  public:
    FEditorViewportLayoutSinglePane() = default;
    ~FEditorViewportLayoutSinglePane() override;

  public:
    void             Initialize(FViewportRect TotalRect) override;
    void             Reset() override;
    void             Resize(FViewportRect NewRect) override;
    TArray<SWindow*> GetLeafWindows() const override;

  private:
    SWindow* WindowA = nullptr;
};
