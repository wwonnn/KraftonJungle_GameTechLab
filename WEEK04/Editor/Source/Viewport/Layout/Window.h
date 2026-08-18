#pragma once

#include "Core/HAL/PlatformTypes.h"
#include "Renderer/SceneView.h"

// 트리 노드
class SWindow
{
public:
    SWindow();
    SWindow(FViewportRect Rect) : ViewportRect(Rect) {}
    virtual ~SWindow() = default;

public:
    FViewportRect const GetViewportRect() const { return ViewportRect; }
    void SetViewportRect(FViewportRect NewRect) { ViewportRect = NewRect; }
    // bool  IsHover(FPoint coord) const;

protected:
    FViewportRect ViewportRect;
};

// Splitter 트리 구조
class SSplitter : public SWindow
{
public:
    SSplitter(float Ratio) : SplitRatio(Ratio) {}
    virtual ~SSplitter()
    { 
          SideLT = nullptr;
          SideRB = nullptr;
    }

public:
    SWindow* const& GetSideLT() const { return SideLT; }
    SWindow* const& GetSideRB() const { return SideRB; }

    void SetSideLT(SWindow* NewWindow) { SideLT = NewWindow; }
    void SetSideRB(SWindow* NewWindow) { SideRB = NewWindow; }

    float GetSplitRatio() { return SplitRatio; }
    void  SetSplitRatio(float Ratio) { SplitRatio = Ratio; }

    virtual void LayoutChildren() = 0;

protected:
    float    SplitRatio = 0.5f;
    SWindow* SideLT = nullptr;  // 왼쪽 자식
    SWindow* SideRB = nullptr;  // 오른쪽 자식
};

class SSplitterH : public SSplitter
{
  public: 
      SSplitterH(float Ratio) : SSplitter(Ratio) {}

  public:
    void LayoutChildren() override;
};

class SSplitterV : public SSplitter
{
  public:
    SSplitterV(float Ratio) : SSplitter(Ratio) {}

  public:
    void LayoutChildren() override;
};