#include "Editor/UI/EditorMainPanel.h"

#include "Editor/Document/EditorDocument.h"

#include <algorithm>

void FEditorMainPanel::OpenDocument(std::unique_ptr<FEditorDocument> Document)
{
    if (!Document)
    {
        return;
    }

    OpenDocuments.push_back(std::move(Document));
}

FEditorDocument* FEditorMainPanel::FindDocumentByTabId(const FEditorTabId& TabId)
{
    for (const std::unique_ptr<FEditorDocument>& Document : OpenDocuments)
    {
        if (Document && Document->GetTabId().Matches(TabId))
        {
            return Document.get();
        }
    }

    return nullptr;
}

const FEditorDocument* FEditorMainPanel::FindDocumentByTabId(const FEditorTabId& TabId) const
{
    for (const std::unique_ptr<FEditorDocument>& Document : OpenDocuments)
    {
        if (Document && Document->GetTabId().Matches(TabId))
        {
            return Document.get();
        }
    }

    return nullptr;
}

FEditorDocument* FEditorMainPanel::GetActiveEditorDocument()
{
    const FEditorTabEntry* ActiveTab = EditorTabs.GetActiveTab();
    return ActiveTab ? FindDocumentByTabId(ActiveTab->Id) : nullptr;
}

const FEditorDocument* FEditorMainPanel::GetActiveEditorDocument() const
{
    const FEditorTabEntry* ActiveTab = EditorTabs.GetActiveTab();
    return ActiveTab ? FindDocumentByTabId(ActiveTab->Id) : nullptr;
}

bool FEditorMainPanel::CloseDocument(const FEditorTabId& TabId)
{
    const auto It = std::remove_if(
        OpenDocuments.begin(),
        OpenDocuments.end(),
        [&TabId](const std::unique_ptr<FEditorDocument>& Document)
        {
            return Document && Document->GetTabId().Matches(TabId);
        });

    if (It == OpenDocuments.end())
    {
        return false;
    }

    OpenDocuments.erase(It, OpenDocuments.end());
    return true;
}
