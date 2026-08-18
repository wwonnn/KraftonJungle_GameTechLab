#pragma once

#include "Core/HAL/PlatformTypes.h"
#include "Core/Math/Matrix.h"
#include "Core/Math/Vector.h"

#include <d3d11.h>

// TODO: Reverse-Z 적용

struct FViewportRect
{
    int32 X = 0;
    int32 Y = 0;
    int32 Width = 0;
    int32 Height = 0;
};

class FSceneView
{
  public:
    void SetViewMatrix(const FMatrix& InViewMatrix)
    {
        ViewMatrix = InViewMatrix;
        RebuildViewProjectionMatrix();
    }

    void SetProjectionMatrix(const FMatrix& InProjectionMatrix)
    {
        ProjectionMatrix = InProjectionMatrix;
        RebuildViewProjectionMatrix();
    }

    void OnResize(const FViewportRect& NewViewRect) { 
        ViewRect = NewViewRect;
        Viewport = {(float)ViewRect.X,
                    (float)ViewRect.Y,
                    (float)ViewRect.Width,
                    (float)ViewRect.Height,
                    0.0f,
                    1.0f};
    }

    void SetViewLocation(const FVector& InViewLocation) { ViewLocation = InViewLocation; }

    void SetClipPlanes(float InNearZ, float InFarZ)
    {
        NearZ = InNearZ;
        FarZ = InFarZ;
    }

    const FMatrix&        GetViewMatrix() const { return ViewMatrix; }
    const FMatrix&        GetProjectionMatrix() const { return ProjectionMatrix; }
    const FMatrix&        GetViewProjectionMatrix() const { return ViewProjectionMatrix; }
    const FVector&        GetViewLocation() const { return ViewLocation; }
    const FViewportRect&  GetViewRect() const { return ViewRect; }
    const D3D11_VIEWPORT& GetViewport() const { return Viewport; }

    float GetCameraWidth() const { return static_cast<float>(ViewRect.Width); }
    float GetCameraHeight() const { return static_cast<float>(ViewRect.Height); }

    int32 GetWorldX(int32 X) const { return X + ViewRect.X; }
    int32 GetWorldY(int32 Y) const { return Y + ViewRect.Y; }

  private:
    void RebuildViewProjectionMatrix() { ViewProjectionMatrix = ViewMatrix * ProjectionMatrix; }

  private:
    FMatrix ViewMatrix;
    FMatrix ProjectionMatrix;
    FMatrix ViewProjectionMatrix;

    FVector ViewLocation;

    float NearZ = 0.1f;
    float FarZ = 1000.0f;

    FViewportRect ViewRect;
    D3D11_VIEWPORT         Viewport;
};