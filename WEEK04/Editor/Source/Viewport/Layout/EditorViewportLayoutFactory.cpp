#include "EditorViewportLayoutFactory.h"

#include "EditorViewportLayoutSinglePane.h"
#include "EditorViewportLayoutTwoPanes.h"
#include "EditorViewportLayoutThreePanes.h"
#include "EditorViewportLayoutFourPanes.h"

FEditorViewportLayout* FEditorViewportLayoutFactory::Create(EViewportLayoutType Type)
{
    switch (Type)
    {
    // ---------------------- Single ----------------------------
    case EViewportLayoutType::Single:
    {
        auto* L = new FEditorViewportLayoutSinglePane();
        L->SetLayoutType(EViewportLayoutType::Single);
        return L;
    }
    // ---------------------- Two -------------------------------
    case EViewportLayoutType::_1l1:
    {
        auto* L = new FEditorViewportLayoutTwoPanes();
        L->SetLayoutType(EViewportLayoutType::_1l1);
        return L;
    }
    case EViewportLayoutType::_1_1:
    {
        auto* L = new FEditorViewportLayoutTwoPanes();
        L->SetLayoutType(EViewportLayoutType::_1_1);
        return L;
    }
    // ---------------------- Three -----------------------------
    case EViewportLayoutType::_1l2:
    {
        auto* L = new FEditorViewportLayoutThreePanes();
        L->SetLayoutType(EViewportLayoutType::_1l2);
        return L;
    }
    case EViewportLayoutType::_2l1:
    {
        auto* L = new FEditorViewportLayoutThreePanes();
        L->SetLayoutType(EViewportLayoutType::_2l1);
        return L;
    }
    case EViewportLayoutType::_1_2:
    {
        auto* L = new FEditorViewportLayoutThreePanes();
        L->SetLayoutType(EViewportLayoutType::_1_2);
        return L;
    }
    case EViewportLayoutType::_2_1:
    {
        auto* L = new FEditorViewportLayoutThreePanes();
        L->SetLayoutType(EViewportLayoutType::_2_1);
        return L;
    }
    // ------------------------ Four ----------------------------
    case EViewportLayoutType::_2X2:
    {
        auto* L = new FEditorViewportLayoutFourPanes();
        L->SetLayoutType(EViewportLayoutType::_2X2);
        return L;
    }
    case EViewportLayoutType::_1l3:
    {
        auto* L = new FEditorViewportLayoutFourPanes();
        L->SetLayoutType(EViewportLayoutType::_1l3);
        return L;
    }
    case EViewportLayoutType::_3l1:
    {
        auto* L = new FEditorViewportLayoutFourPanes();
        L->SetLayoutType(EViewportLayoutType::_3l1);
        return L;
    }
    case EViewportLayoutType::_1_3:
    {
        auto* L = new FEditorViewportLayoutFourPanes();
        L->SetLayoutType(EViewportLayoutType::_1_3);
        return L;
    }
    case EViewportLayoutType::_3_1:
    {
        auto* L = new FEditorViewportLayoutFourPanes();
        L->SetLayoutType(EViewportLayoutType::_3_1);
        return L;
    }
    default:
        return nullptr;
    }
}