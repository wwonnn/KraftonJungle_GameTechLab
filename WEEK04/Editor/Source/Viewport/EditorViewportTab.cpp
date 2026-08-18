#include "EditorViewportTab.h"
#include "Viewport/Layout/EditorViewportLayoutFourPanes.h"

#include "imgui.h"
#include "imgui_internal.h"

SEditorViewportTab::SEditorViewportTab() {}

SEditorViewportTab::~SEditorViewportTab() {}

void SEditorViewportTab::Construct()
{ 
    // 현재 최대 4개 Viewport만 가능
    for (int32 i = 0; i < 4; i++)
    {
        FViewport*             NewViewport = new FViewport();
        FSceneView*            NewSceneView = new FSceneView();
        FEditorViewportClient* NewViewportClient = new FEditorViewportClient();

        NewViewportClient->Create();
        NewViewport->SetSceneView(NewSceneView);
        NewViewport->SetViewportClient(NewViewportClient);

        Viewports.push_back(NewViewport);
        ViewportClients.push_back(NewViewportClient);
    }

    SetLayout(EViewportLayoutType::_2X2);
}

void SEditorViewportTab::Release()
{
    for (auto* Viewport : Viewports)
    {
        Viewport->Release();
        delete Viewport;
    }

    for (auto* P : ControlPanels)
    {
        delete P;
    }

    delete ViewportLayout;
    ViewportLayout = nullptr;
}

void SEditorViewportTab::OnResize(FViewportRect WindowRect, bool Force)
{ 
    if (!Force && WindowRect.X == CurrentRect.X && WindowRect.Y == CurrentRect.Y &&
        WindowRect.Width == CurrentRect.Width && WindowRect.Height == CurrentRect.Height)
        return;

    CurrentRect = WindowRect;
    ViewportLayout->Resize(WindowRect);

    auto windows = ViewportLayout->GetLeafWindows();
    for (int i = 0; i < windows.size(); i++)
    {
        if (Viewports[i]->IsValid())
        {
            Viewports[i]->GetSceneView()->OnResize(windows[i]->GetViewportRect());
            Viewports[i]->GetViewportClient()->OnResize(windows[i]->GetViewportRect().Width,
                                                         windows[i]->GetViewportRect().Height);
        }
    }
}

void SEditorViewportTab::SetLayout(EViewportLayoutType NewType)
{
    delete ViewportLayout;
    ViewportLayout = nullptr;

    AdjustViewportCount(NewType);

    ViewportLayout = FEditorViewportLayoutFactory::Create(NewType);
    ViewportLayout->Initialize(CurrentRect);

    OnResize(CurrentRect, true);
    CurrentLayoutType = NewType;
}

void SEditorViewportTab::AdjustViewportCount(EViewportLayoutType NewType) 
{
    int32 Required = GetRequiredViewportCount(NewType);

    for (int32 i = 0; i < (int32)Viewports.size(); i++)
    {
        if (i < Required)
        {
            if (!Viewports[i]->IsValid())
                Viewports[i]->SetValid(true);
        }
        else
        {
            Viewports[i]->SetValid(false);
        }
    }
}

void SEditorViewportTab::InitializeControlPanels(FEditorContext* Context)
{
    for (int i = 0; i < Viewports.size(); i++)
    {
        FControlPanel* Panel = new FControlPanel();
        Panel->Initialize(Context, nullptr);
        Panel->SetViewportIndex(i);
        ControlPanels.push_back(Panel);
    }
}

void SEditorViewportTab::DrawControlPanels() 
{
    if (auto* Root = dynamic_cast<SSplitter*>(ViewportLayout->GetRootSplitter()))
        DrawSplitters(Root, nullptr);

    for (int i = 0; i < Viewports.size(); i++)
    {
        if (Viewports[i]->IsValid())
            ControlPanels[i]->Draw(); 
    }
}

void SEditorViewportTab::DrawSplitters(SSplitter* Splitter, SSplitter* Parent)
{
    if (Splitter == nullptr)
        return;

    FViewportRect Rect = Splitter->GetViewportRect();
    ImGuiIO&      IO = ImGui::GetIO();
    ImDrawList*   DrawList = ImGui::GetBackgroundDrawList();

    if (SSplitterH* H = dynamic_cast<SSplitterH*>(Splitter))
    {
        float SplitterX = Rect.X + Rect.Width * H->GetSplitRatio();

        ImVec2 Min = ImVec2(SplitterX - 3.0f, Rect.Y);
        ImVec2 Max = ImVec2(SplitterX + 3.0f, Rect.Y + Rect.Height);

        bool bHovered = IO.MousePos.x >= Min.x && IO.MousePos.x <= Max.x &&
                        IO.MousePos.y >= Min.y && IO.MousePos.y <= Max.y;

        bool bThisDragging = (DraggingSplitter == Splitter);

        if (bHovered || bThisDragging)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        if (bHovered && ImGui::IsMouseClicked(0) && DraggingSplitter == nullptr)
            DraggingSplitter = Splitter;

        if (bThisDragging)
        {
            if (ImGui::IsMouseDown(0))
            {
                float OldRBAbsWidth = Rect.Width * (1.0f - H->GetSplitRatio());
                float NewRatio = H->GetSplitRatio() + IO.MouseDelta.x / Rect.Width;
                NewRatio = std::clamp(NewRatio, 0.1f, 0.9f);
                H->SetSplitRatio(NewRatio);
                H->LayoutChildren();

                // 2X2 형제 SSplitterH 동기화
                if (Parent != nullptr && dynamic_cast<SSplitterV*>(Parent))
                {
                    SSplitter* Sibling = nullptr;
                    if (Parent->GetSideLT() == Splitter)
                        Sibling = dynamic_cast<SSplitter*>(Parent->GetSideRB());
                    else if (Parent->GetSideRB() == Splitter)
                        Sibling = dynamic_cast<SSplitter*>(Parent->GetSideLT());

                    if (SSplitterH* SiblingH = dynamic_cast<SSplitterH*>(Sibling))
                    {
                        SiblingH->SetSplitRatio(NewRatio);
                        SiblingH->LayoutChildren();
                    }
                }

                if (SSplitterH* ChildH = dynamic_cast<SSplitterH*>(H->GetSideRB()))
                {
                    float NewRBAbsWidth = Rect.Width * (1.0f - NewRatio);
                    if (NewRBAbsWidth > 0)
                    {
                        float ChildRBAbsWidth = OldRBAbsWidth * (1.0f - ChildH->GetSplitRatio());
                        float NewChildRatio = 1.0f - (ChildRBAbsWidth / NewRBAbsWidth);
                        ChildH->SetSplitRatio(std::clamp(NewChildRatio, 0.1f, 0.9f));
                        ChildH->LayoutChildren();
                    }
                }

                OnResize(CurrentRect, true);
            }
            else
                DraggingSplitter = nullptr;
        }

        ImU32 Color = bThisDragging ? IM_COL32(150, 150, 150, 255)
                      : bHovered    ? IM_COL32(100, 100, 100, 255)
                                    : IM_COL32(60, 60, 60, 255);
        DrawList->AddRectFilled(ImVec2(SplitterX - 1.0f, Rect.Y),
                                ImVec2(SplitterX + 1.0f, Rect.Y + Rect.Height), Color);
    }
    else if (SSplitterV* V = dynamic_cast<SSplitterV*>(Splitter))
    {
        float SplitterY = Rect.Y + Rect.Height * V->GetSplitRatio();

        ImVec2 Min = ImVec2(Rect.X, SplitterY - 3.0f);
        ImVec2 Max = ImVec2(Rect.X + Rect.Width, SplitterY + 3.0f);

        bool bHovered = IO.MousePos.x >= Min.x && IO.MousePos.x <= Max.x &&
                        IO.MousePos.y >= Min.y && IO.MousePos.y <= Max.y;

        bool bThisDragging = (DraggingSplitter == Splitter);

        if (bHovered || bThisDragging)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

        if (bHovered && ImGui::IsMouseClicked(0) && DraggingSplitter == nullptr)
            DraggingSplitter = Splitter;

        if (bThisDragging)
        {
            if (ImGui::IsMouseDown(0))
            {
                // 드래그 전 SideRB(Splitter2)의 절대 높이 저장
                float OldRBAbsHeight = Rect.Height * (1.0f - V->GetSplitRatio());

                float NewRatio = V->GetSplitRatio() + IO.MouseDelta.y / Rect.Height;
                NewRatio = std::clamp(NewRatio, 0.1f, 0.9f);
                V->SetSplitRatio(NewRatio);
                V->LayoutChildren();

                if (SSplitterV* ChildV = dynamic_cast<SSplitterV*>(V->GetSideRB()))
                {
                    float NewRBAbsHeight = Rect.Height * (1.0f - NewRatio);
                    if (NewRBAbsHeight > 0)
                    {
                        // SideRB(WindowD, 3번째)의 절대 높이를 고정
                        float ChildRBAbsHeight = OldRBAbsHeight * (1.0f - ChildV->GetSplitRatio());
                        float NewChildRatio = 1.0f - (ChildRBAbsHeight / NewRBAbsHeight);
                        ChildV->SetSplitRatio(std::clamp(NewChildRatio, 0.1f, 0.9f));
                        ChildV->LayoutChildren();
                    }
                }

                OnResize(CurrentRect, true);
            }
            else
                DraggingSplitter = nullptr;
        }

        ImU32 Color = bThisDragging ? IM_COL32(150, 150, 150, 255)
                      : bHovered    ? IM_COL32(100, 100, 100, 255)
                                    : IM_COL32(60, 60, 60, 255);
        DrawList->AddRectFilled(ImVec2(Rect.X, SplitterY - 1.0f),
                                ImVec2(Rect.X + Rect.Width, SplitterY + 1.0f), Color);
    }

    if (auto* Child = dynamic_cast<SSplitter*>(Splitter->GetSideLT()))
        DrawSplitters(Child, Splitter);
    if (auto* Child = dynamic_cast<SSplitter*>(Splitter->GetSideRB()))
        DrawSplitters(Child, Splitter);
}

const void SEditorViewportTab::GetSplitterRatios(float OutRatios[3]) const
{
    for (int i = 0; i < 3; ++i)
        OutRatios[i] = 0.5f;

    int Index = 0;
    std::function<void(SSplitter*)> GatherRatios = [&](SSplitter* S)
    {
        if (!S || Index >= 3)
            return;
        OutRatios[Index++] = S->GetSplitRatio();
        GatherRatios(dynamic_cast<SSplitter*>(S->GetSideLT()));
        GatherRatios(dynamic_cast<SSplitter*>(S->GetSideRB()));
    };

    if (ViewportLayout)
        GatherRatios(dynamic_cast<SSplitter*>(ViewportLayout->GetRootSplitter()));
}

void SEditorViewportTab::SetSplitterRatios(const float InRatios[3]) 
{
    int Index = 0;
    std::function<void(SSplitter*)> ApplyRatios = [&](SSplitter* S)
    {
        if (!S || Index >= 3)
            return;
        S->SetSplitRatio(InRatios[Index++]);
        S->LayoutChildren();
        ApplyRatios(dynamic_cast<SSplitter*>(S->GetSideLT()));
        ApplyRatios(dynamic_cast<SSplitter*>(S->GetSideRB()));
    };

    if (ViewportLayout)
        ApplyRatios(dynamic_cast<SSplitter*>(ViewportLayout->GetRootSplitter()));
}

