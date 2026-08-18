#pragma once

#include "EditorViewportLayout.h"

class FEditorViewportLayoutThreePanes : public FEditorViewportLayout
{
  public:
    ~FEditorViewportLayoutThreePanes() override;

  public:
    void             Initialize(FViewportRect TotalRect) override;
    void             Reset() override;
    void             Resize(FViewportRect NewRect) override;
    TArray<SWindow*> GetLeafWindows() const override;

  private:
    void Build_1l2Tree();
    void Build_2l1Tree();
    void Build_1_2Tree();
    void Build_2_1Tree();

  private:
    SWindow* WindowA = nullptr;
    SWindow* WindowB = nullptr;
    SWindow* WindowC = nullptr;

    SSplitter* Splitter1 = nullptr;
};
