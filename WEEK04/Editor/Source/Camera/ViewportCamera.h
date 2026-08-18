#pragma once

#include "Core/CoreMinimal.h"

enum class EViewportProjectionType
{
    Perspective,
    Orthographic
};

enum class EViewportOrthographicType
{
    None, // GUI용 패딩
    Top,
    Bottom,
    Left,
    Right,
    Front,
    Back
};

struct FPerspectiveInfo
{
    FVector Location;
    FQuat   Rotation;
    float   FOV;
    float   NearPlane;
    float   FarPlane;
};

class FViewportCamera
{
  public:
    FViewportCamera() = default;
    ~FViewportCamera() = default;

    void SetLocation(const FVector& InLocation);
    void SetRotation(const FQuat& InRotation);
    void SetRotation(const FRotator& InRotation);

    const FVector& GetLocation() const { return Location; }
    const FQuat&   GetRotation() const { return Rotation; }

    const FVector& GetOrthoForward() const { return OrthoForward; }
    const FVector& GetOrthoUp() const { return OrthoUp; }

    FVector GetForwardVector() const;
    FVector GetRightVector() const;
    FVector GetUpVector() const;

    FMatrix GetViewMatrix() const;
    FMatrix GetProjectionMatrix() const;
    FMatrix GetViewProjectionMatrix() const;

    void                    SetProjectionType(EViewportProjectionType InType);
    EViewportProjectionType GetProjectionType() const { return ProjectionType; }

    void                    SetOrthographicType(EViewportOrthographicType InType);
    EViewportOrthographicType GetOrthographicType() const { return OrthographicType; }

    void SetFOV(float InFOV);
    void SetNearPlane(float InNear);
    void SetFarPlane(float InFar);
    void SetOrthoHeight(float InHeight);

    float GetFOV() const { return FOV; }
    float GetNearPlane() const { return NearPlane; }
    float GetFarPlane() const { return FarPlane; }
    float GetOrthoHeight() const { return OrthoHeight; }

    void OnResize(uint32 InWidth, uint32 InHeight);

    uint32 GetWidth() const { return Width; }
    uint32 GetHeight() const { return Height; }
    float  GetAspectRatio() const { return AspectRatio; }

    const FPerspectiveInfo& GetPerspectiveInfo() const { return CachedPerspectiveInfo; }

  private:
    void MarkViewDirty() { bIsViewDirty = true; }
    void MarkProjectionDirty() { bIsProjectionDirty = true; }

  private:
    FVector Location = FVector::Zero();
    FQuat   Rotation = FQuat::Identity;
    FPerspectiveInfo CachedPerspectiveInfo;

    EViewportProjectionType ProjectionType = EViewportProjectionType::Perspective;
    EViewportOrthographicType OrthographicType = EViewportOrthographicType::Top;

    uint32 Width = 1920;
    uint32 Height = 1080;
    float  AspectRatio = 16.0f / 9.0f;

    float FOV = 3.141592f;
    float NearPlane = 0.1f;
    float FarPlane = 2000.0f;

    float OrthoHeight = 100.0f;

    mutable FVector OrthoForward = FVector::Zero();
    mutable FVector OrthoUp = FVector::Zero();

    mutable FMatrix CachedViewMatrix;
    mutable FMatrix CachedProjectionMatrix;
    mutable bool    bIsViewDirty = true;
    mutable bool    bIsProjectionDirty = true;
};
