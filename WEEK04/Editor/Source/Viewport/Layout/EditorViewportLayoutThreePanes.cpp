#include "EditorViewportLayoutThreePanes.h"

FEditorViewportLayoutThreePanes::~FEditorViewportLayoutThreePanes() { Reset(); }

void FEditorViewportLayoutThreePanes::Initialize(FViewportRect TotalRect)
{
    Reset();

    WindowA = new SWindow();
    WindowB = new SWindow();
    WindowC = new SWindow();

    // Root 설정
    if (Type == EViewportLayoutType::_1l2 || Type == EViewportLayoutType::_2l1)
    {
        SSplitterH* RootSH = new SSplitterH(0.5f);
        RootSH->SetViewportRect(TotalRect);
        RootSplitter = RootSH;
    }
    else
    {
        SSplitterV* RootSV = new SSplitterV(0.5f);
        RootSV->SetViewportRect(TotalRect);
        RootSplitter = RootSV;
    }

    // Build Tree
    if (Type == EViewportLayoutType::_1l2)
    {
        Build_1l2Tree();
    }
    else if (Type == EViewportLayoutType::_2l1)
    {
        Build_2l1Tree();
    }
    else if (Type == EViewportLayoutType::_1_2)
    {
        Build_1_2Tree();
    }
    else if (Type == EViewportLayoutType::_2_1)
    {
        Build_2_1Tree();
    }
}

void FEditorViewportLayoutThreePanes::Reset()
{
    delete WindowA;
    WindowA = nullptr;
    delete WindowB;
    WindowB = nullptr;
    delete WindowC;
    WindowC = nullptr;
    delete Splitter1;
    Splitter1 = nullptr;
    delete RootSplitter;
    RootSplitter = nullptr;
}

void FEditorViewportLayoutThreePanes::Resize(FViewportRect NewRect)
{
    RootSplitter->SetViewportRect(NewRect);
    RootSplitter->LayoutChildren();

    Splitter1->LayoutChildren();
}

TArray<SWindow*> FEditorViewportLayoutThreePanes::GetLeafWindows() const
{
    return {WindowA, WindowB, WindowC};
}

void FEditorViewportLayoutThreePanes::Build_1l2Tree()
{
    //  SSplitterH (root)
    //  ├─ SideLT: WindowA (좌)
    //  └─ SideRB: SSplitterV
    //      ├─ SideLT: WindowB  (우상)
    //      └─ SideRB: WindowC  (우하)

    if (RootSplitter == nullptr)
        return;

    Splitter1 = new SSplitterV(0.5f);

    RootSplitter->SetSideLT(WindowA);
    RootSplitter->SetSideRB(Splitter1);
    RootSplitter->LayoutChildren();

    Splitter1->SetSideLT(WindowB);
    Splitter1->SetSideRB(WindowC);
    Splitter1->LayoutChildren();
}

void FEditorViewportLayoutThreePanes::Build_2l1Tree() 
{
    //  SSplitterH (root)
    //  ├─ SideLT: SSplitterV
    //  │  ├─ SideLT: WindowA  (좌상)
    //  │  └─ SideRB: WindowB  (좌하)
    //  └─ SideRB: WindowC (우)


    if (RootSplitter == nullptr)
        return;

    Splitter1 = new SSplitterV(0.5f);

    RootSplitter->SetSideLT(Splitter1);
    RootSplitter->SetSideRB(WindowA);
    RootSplitter->LayoutChildren();

    Splitter1->SetSideLT(WindowB);
    Splitter1->SetSideRB(WindowC);
    Splitter1->LayoutChildren();
}

void FEditorViewportLayoutThreePanes::Build_1_2Tree() 
{
    //  SSplitterV (root)
    //  ├─ SideLT: WindowA (상)
    //  └─ SideRB: SSplitterH
    //      ├─ SideLT: WindowB  (좌하)
    //      └─ SideRB: WindowC  (우하)

    if (RootSplitter == nullptr)
        return;

    Splitter1 = new SSplitterH(0.5f);

    RootSplitter->SetSideLT(WindowA);
    RootSplitter->SetSideRB(Splitter1);
    RootSplitter->LayoutChildren();

    Splitter1->SetSideLT(WindowB);
    Splitter1->SetSideRB(WindowC);
    Splitter1->LayoutChildren();
}

void FEditorViewportLayoutThreePanes::Build_2_1Tree() 
{
    //  SSplitterV (root)
    //  ├─ SideLT: SSplitterH
    //  │  ├─ SideLT: WindowA  (좌상)
    //  │  └─ SideRB: WindowB  (우상)
    //  └─ SideRB: WindowC (하)

    if (RootSplitter == nullptr)
        return;

    Splitter1 = new SSplitterH(0.5f);

    RootSplitter->SetSideLT(Splitter1);
    RootSplitter->SetSideRB(WindowA);
    RootSplitter->LayoutChildren();

    Splitter1->SetSideLT(WindowB);
    Splitter1->SetSideRB(WindowC);
    Splitter1->LayoutChildren();
}
