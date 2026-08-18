#pragma once

#include "Panel.h"

class AActor;
class UStaticMesh;
class UMaterial;

namespace Engine::Component
{
    class USceneComponent;
    class UStaticMeshComponent;
}

struct FStaticMeshEntry
{
    FString      Name;
    FString      SourcePath;
    UStaticMesh* Ptr = nullptr;
};

struct FMaterialEntry
{
    FString      Name;
    FString      SourcePath;
    UMaterial* Ptr = nullptr;
};

class FPropertiesPanel : public IPanel
{
  public:
    const wchar_t* GetPanelID() const override;
    const wchar_t* GetDisplayName() const override;
    bool ShouldOpenByDefault() const override { return true; }
    int32 GetWindowMenuOrder() const override { return 10; }

    void Draw() override;

    void SetTarget(const FVector& Location, const FVector& Rotation, const FVector& Scale);

  private:
    AActor* ResolveSelectedActor() const;
    Engine::Component::USceneComponent* ResolveTargetComponent(AActor*& OutSelectedActor) const;
    void DrawNoSelectionState() const;
    void DrawMultipleSelectionState() const;
    void DrawUnsupportedSelectionState() const;
    void DrawSelectionSummary(AActor* SelectedActor,
                              Engine::Component::USceneComponent* TargetComponent) const;
    void DrawComponentHierarchy(AActor* SelectedActor,
                                Engine::Component::USceneComponent* TargetComponent) const;
    void DrawComponentNode(AActor* OwnerActor, Engine::Component::USceneComponent* Component,
                           Engine::Component::USceneComponent* TargetComponent) const;
    void SyncEditTransformFromTarget(Engine::Component::USceneComponent* TargetComponent);
    void DrawTransformEditor(Engine::Component::USceneComponent* TargetComponent);
    void DrawComponentPropertyEditor(Engine::Component::USceneComponent* TargetComponent);
    void DrawStaticMeshEditor(Engine::Component::USceneComponent* TargetComponent);
    void DrawMaterialsEditor(Engine::Component::USceneComponent* TargetComponent);
    void ResetAssetPathEditState();
    void RebuildAssetComboCache();

  private:
    FVector EditLocation = { 0.f, 0.f, 0.f };
    FVector EditRotation = { 0.f, 0.f, 0.f };
    FVector EditScale = { 1.f, 1.f, 1.f };
    Engine::Component::USceneComponent* CachedTargetComponent = nullptr;
    TMap<FString, FString> AssetPathEditBuffers;
    
    TArray<FStaticMeshEntry> CachedStaticMeshes;
    TArray<FMaterialEntry>   CachedMaterials;
    bool bDirty = true;
};
