#pragma once

#include "EditorViewportLayout.h"

class FEditorViewportLayoutTwoPanes : public FEditorViewportLayout
{
  public:
    ~FEditorViewportLayoutTwoPanes() override;

  public:
    void             Initialize(FViewportRect TotalRect) override;
    void             Reset() override;
    void             Resize(FViewportRect NewRect) override;
    TArray<SWindow*> GetLeafWindows() const override;

  private:
    void Build_1l1Tree();
    void Build_1_1Tree();

  private:
    SWindow* WindowA = nullptr;
    SWindow* WindowB = nullptr;
};
