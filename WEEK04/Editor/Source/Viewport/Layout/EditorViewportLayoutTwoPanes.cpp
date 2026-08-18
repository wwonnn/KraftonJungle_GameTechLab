#include "EditorViewportLayoutTwoPanes.h"

FEditorViewportLayoutTwoPanes::~FEditorViewportLayoutTwoPanes() { Reset(); }

void FEditorViewportLayoutTwoPanes::Initialize(FViewportRect TotalRect) 
{
    Reset();

    WindowA = new SWindow();
    WindowB = new SWindow();

    // Root 설정
    if (Type == EViewportLayoutType::_1l1)
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
    if (Type == EViewportLayoutType::_1l1)
    {
        Build_1l1Tree();
    }
    else if (Type == EViewportLayoutType::_1_1)
    {
        Build_1_1Tree();
    }
}

void FEditorViewportLayoutTwoPanes::Reset() 
{
    delete WindowA;
    WindowA = nullptr;
    delete WindowB;
    WindowB = nullptr;
    delete RootSplitter;
    RootSplitter = nullptr;
}

void FEditorViewportLayoutTwoPanes::Resize(FViewportRect NewRect) 
{
    RootSplitter->SetViewportRect(NewRect);
    RootSplitter->LayoutChildren();
}

TArray<SWindow*> FEditorViewportLayoutTwoPanes::GetLeafWindows() const
{
    return {WindowA, WindowB};
}

void FEditorViewportLayoutTwoPanes::Build_1l1Tree() 
{
    //  SSplitterH (root)
    //  ├─ SideLT: WindowA (좌)
    //  └─ SideRB: WindowB (우)

    if (RootSplitter == nullptr)
        return;

    RootSplitter->SetSideLT(WindowA);
    RootSplitter->SetSideRB(WindowB);
    RootSplitter->LayoutChildren();
}

void FEditorViewportLayoutTwoPanes::Build_1_1Tree() 
{
    //  SSplitterV (root)
    //  ├─ SideLT: WindowA (상)
    //  └─ SideRB: WindowB (하)

    if (RootSplitter == nullptr)
        return;

    RootSplitter->SetSideLT(WindowA);
    RootSplitter->SetSideRB(WindowB);
    RootSplitter->LayoutChildren();
}
