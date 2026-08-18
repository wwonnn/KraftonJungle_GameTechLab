#pragma once

#include "Core/CoreMinimal.h"
#include "Window.h"
#include "EditorViewportLayoutType.h"

class FEditorViewportLayout
{
  public:
    virtual ~FEditorViewportLayout() = default;
    
    // 초기 트리 구성
    virtual void Initialize(FViewportRect TotalRect) = 0;

    // 초기화 (메모리 해제)
    virtual void Reset() = 0;

    // 창 크기 변경 시 재구성
    virtual void Resize(FViewportRect NewRect) = 0;

    // 리프 노드 반환 (최종 결과)
    virtual TArray<SWindow*> GetLeafWindows() const = 0;

    void SetLayoutType(EViewportLayoutType NewType) { Type = NewType; }
    SSplitter* const& GetRootSplitter() const { return RootSplitter; }

protected:
    EViewportLayoutType Type = EViewportLayoutType::_1l3;
    SSplitter* RootSplitter = nullptr;
};