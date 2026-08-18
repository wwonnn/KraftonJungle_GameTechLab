#include "EditorViewportLayoutSinglePane.h"

FEditorViewportLayoutSinglePane::~FEditorViewportLayoutSinglePane() { Reset(); }

void FEditorViewportLayoutSinglePane::Initialize(FViewportRect TotalRect) 
{
    Reset();

    WindowA = new SWindow();
    RootSplitter = nullptr;

    Type = EViewportLayoutType::Single;
    WindowA->SetViewportRect(TotalRect);
}

void FEditorViewportLayoutSinglePane::Reset() 
{
    delete WindowA;
    WindowA = nullptr;
}

void FEditorViewportLayoutSinglePane::Resize(FViewportRect NewRect) 
{
    if (WindowA != nullptr)
        WindowA->SetViewportRect(NewRect);
}

TArray<SWindow*> FEditorViewportLayoutSinglePane::GetLeafWindows() const
{ 
    return { WindowA };
}
