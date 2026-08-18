#include "Window.h"

SWindow::SWindow() {}

void SSplitterH::LayoutChildren() 
{
    int32 SplitX = ViewportRect.Width * SplitRatio;

    // Left
    SideLT->SetViewportRect({ViewportRect.X, ViewportRect.Y, SplitX, ViewportRect.Height});

    // Right
    SideRB->SetViewportRect({ViewportRect.X + SplitX, ViewportRect.Y, ViewportRect.Width - SplitX,
                             ViewportRect.Height});
}

void SSplitterV::LayoutChildren() 
{
    int32 SplitY = ViewportRect.Height * SplitRatio;

    // Top
    SideLT->SetViewportRect({ViewportRect.X, ViewportRect.Y, ViewportRect.Width, SplitY});

    // Bottom
    SideRB->SetViewportRect(
        {ViewportRect.X, ViewportRect.Y + SplitY, ViewportRect.Width, ViewportRect.Height - SplitY});
}