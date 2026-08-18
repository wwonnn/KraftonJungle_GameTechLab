#include "EditorViewportLayoutFourPanes.h"

FEditorViewportLayoutFourPanes::~FEditorViewportLayoutFourPanes() { Reset(); }

void FEditorViewportLayoutFourPanes::Initialize(FViewportRect TotalRect) 
{
    Reset();

    WindowA = new SWindow();
    WindowB = new SWindow();
    WindowC = new SWindow();
    WindowD = new SWindow();

    // Root 설정
    if (Type == EViewportLayoutType::_1_3 || Type == EViewportLayoutType::_3_1)
    {
        SSplitterV* RootSV = new SSplitterV(0.5f);
        RootSV->SetViewportRect(TotalRect);
        RootSplitter = RootSV;
    }
    else
    {
        SSplitterH* RootSH = new SSplitterH(0.5f);
        RootSH->SetViewportRect(TotalRect);
        RootSplitter = RootSH;
    }

    // Build Tree
    if (Type == EViewportLayoutType::_2X2)
    {
        Build_2X2Tree();
    }
    else if (Type == EViewportLayoutType::_1l3)
    {
        Build_1l3Tree();
    }
    else if (Type == EViewportLayoutType::_3l1)
    {
        Build_3l1Tree();
    }
    else if (Type == EViewportLayoutType::_1_3)
    {
        Build_1_3Tree();
    }
    else if (Type == EViewportLayoutType::_3_1)
    {
        Build_3_1Tree();
    }
}

void FEditorViewportLayoutFourPanes::Reset() 
{
    delete WindowA;
    WindowA = nullptr;
    delete WindowB;
    WindowB = nullptr;
    delete WindowC;
    WindowC = nullptr;
    delete WindowD;
    WindowD = nullptr;
    delete Splitter1;
    Splitter1 = nullptr;
    delete Splitter2;
    Splitter2 = nullptr;
    delete RootSplitter;
    RootSplitter = nullptr;
}

void FEditorViewportLayoutFourPanes::Resize(FViewportRect NewRect) 
{
    RootSplitter->SetViewportRect(NewRect);
    RootSplitter->LayoutChildren();

    Splitter1->LayoutChildren();
    Splitter2->LayoutChildren();
}

TArray<SWindow*> FEditorViewportLayoutFourPanes::GetLeafWindows() const
{
    return {WindowA, WindowB, WindowC, WindowD};
}

void FEditorViewportLayoutFourPanes::Build_2X2Tree()
{
    //  SSplitterH (root)
    //  ├─ SideLT: SSplitterV
    //  │   ├─ SideLT: WindowA  (좌상)
    //  │   └─ SideRB: WindowB  (좌하)
    //  └─ SideRB: SSplitterV
    //      ├─ SideLT: WindowC  (우상)
    //      └─ SideRB: WindowD  (우하)

    if (RootSplitter == nullptr)
        return;

    Splitter1 = new SSplitterV(0.5f);
    Splitter2 = new SSplitterV(0.5f);

    RootSplitter->SetSideLT(Splitter1);
    RootSplitter->SetSideRB(Splitter2);
    RootSplitter->LayoutChildren();

    Splitter1->SetSideLT(WindowA);
    Splitter1->SetSideRB(WindowB);
    Splitter1->LayoutChildren();

    Splitter2->SetSideLT(WindowC);
    Splitter2->SetSideRB(WindowD);
    Splitter2->LayoutChildren();
}

void FEditorViewportLayoutFourPanes::Build_1l3Tree() 
{
    // SSplitterH (root)
    // ├─ SideLT: WindowA          (좌)
    // └─ SideRB: SSplitterV
    //     ├─ SideLT: WindowB      (우상)
    //     └─ SideRB: SSplitterV
    //         ├─ SideLT: WindowC  (우중)
    //         └─ SideRB: WindowD  (우하)

    if (RootSplitter == nullptr)
        return;

    Splitter1 = new SSplitterV(0.33f);
    Splitter2 = new SSplitterV(0.5f);

    RootSplitter->SetSideLT(WindowA);
    RootSplitter->SetSideRB(Splitter1);
    RootSplitter->LayoutChildren();

    Splitter1->SetSideLT(WindowB);
    Splitter1->SetSideRB(Splitter2);
    Splitter1->LayoutChildren();

    Splitter2->SetSideLT(WindowC);
    Splitter2->SetSideRB(WindowD);
    Splitter2->LayoutChildren();
}

void FEditorViewportLayoutFourPanes::Build_3l1Tree() 
{
    // SSplitterH (root)
    // ├─ SideLT: SSplitterV
    // │   ├─ SideLT: WindowA  (좌상)
    // │   └─ SideRB: SSplitterV
    // │       ├─ SideLT: WindowB  (좌중)
    // │       └─ SideRB: WindowC  (좌하)
    // └─ SideRB: WindowD          (우)

    if (RootSplitter == nullptr)
        return;

    Splitter1 = new SSplitterV(0.33f);
    Splitter2 = new SSplitterV(0.5f);

    RootSplitter->SetSideLT(Splitter1);
    RootSplitter->SetSideRB(WindowD);
    RootSplitter->LayoutChildren();

    Splitter1->SetSideLT(WindowA);
    Splitter1->SetSideRB(Splitter2);
    Splitter1->LayoutChildren();

    Splitter2->SetSideLT(WindowB);
    Splitter2->SetSideRB(WindowC);
    Splitter2->LayoutChildren();
}

void FEditorViewportLayoutFourPanes::Build_1_3Tree() 
{
    // SSplitterV (root)
    // ├─ SideLT: WindowA          (상)
    // └─ SideRB: SSplitterH
    //     ├─ SideLT: WindowB      (하좌)
    //     └─ SideRB: SSplitterH
    //         ├─ SideLT: WindowC  (하중)
    //         └─ SideRB: WindowD  (하우)

    if (RootSplitter == nullptr)
        return;

    Splitter1 = new SSplitterH(0.33f);
    Splitter2 = new SSplitterH(0.5f);

    RootSplitter->SetSideLT(WindowA);
    RootSplitter->SetSideRB(Splitter1);
    RootSplitter->LayoutChildren();

    Splitter1->SetSideLT(WindowB);
    Splitter1->SetSideRB(Splitter2);
    Splitter1->LayoutChildren();

    Splitter2->SetSideLT(WindowC);
    Splitter2->SetSideRB(WindowD);
    Splitter2->LayoutChildren();
}

void FEditorViewportLayoutFourPanes::Build_3_1Tree() 
{
    // SSplitterV (root)
    // ├─ SideLT: SSplitterH
    // │   ├─ SideLT: WindowA      (상좌)
    // │   └─ SideRB: SSplitterH
    // │       ├─ SideLT: WindowB  (상중)
    // │       └─ SideRB: WindowC  (상우)
    // └─ SideRB: WindowD          (하)

    if (RootSplitter == nullptr)
        return;

    Splitter1 = new SSplitterH(0.33f);
    Splitter2 = new SSplitterH(0.5f);

    RootSplitter->SetSideLT(Splitter1);
    RootSplitter->SetSideRB(WindowD);
    RootSplitter->LayoutChildren();

    Splitter1->SetSideLT(WindowA);
    Splitter1->SetSideRB(Splitter2);
    Splitter1->LayoutChildren();

    Splitter2->SetSideLT(WindowB);
    Splitter2->SetSideRB(WindowC);
    Splitter2->LayoutChildren();
}


