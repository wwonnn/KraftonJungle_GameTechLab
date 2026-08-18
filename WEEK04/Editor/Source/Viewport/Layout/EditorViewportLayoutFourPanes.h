#pragma once

#include "EditorViewportLayout.h"

class FEditorViewportLayoutFourPanes : public FEditorViewportLayout
{
public:
    ~FEditorViewportLayoutFourPanes() override;

public:
    void             Initialize(FViewportRect TotalRect) override;
    void             Reset() override;
    void             Resize(FViewportRect NewRect) override;
    TArray<SWindow*> GetLeafWindows() const override;

private:
    void Build_2X2Tree();
    void Build_1l3Tree();
    void Build_3l1Tree();
    void Build_1_3Tree();
    void Build_3_1Tree();

private:
    SWindow* WindowA = nullptr; 
    SWindow* WindowB = nullptr;
    SWindow* WindowC = nullptr;
    SWindow* WindowD = nullptr;

    SSplitter* Splitter1 = nullptr;
    SSplitter* Splitter2 = nullptr;
};