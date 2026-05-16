#pragma once

#include "Editor/UI/EditorCommandContext.h"
#include "Editor/UI/EditorTabManager.h"

class FEditorDocument
{
public:
    virtual ~FEditorDocument() = default;

    virtual const FEditorTabId& GetTabId() const = 0;
    virtual const FString& GetTabLabel() const = 0;

    virtual void RenderEmbedded(float DeltaTime) = 0;
    virtual void RenderToolbar() = 0;
    virtual void BuildCommandList(FEditorCommandList& OutCommands) = 0;

    virtual bool CanSave() const { return false; }
    virtual bool Save() { return false; }
    virtual bool CanClose() const { return true; }
    virtual bool IsDetachedSupported() const { return false; }
};
