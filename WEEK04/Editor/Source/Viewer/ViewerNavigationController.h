#pragma once

#include "Core/CoreMinimal.h"
#include "Camera/ViewportCamera.h"

// 뷰어 전용 카메라 컨트롤러입니다. (Orbit, Pan, Zoom만 지원)
class FViewerNavigationController
{
  public:
    void SetCamera(FViewportCamera* InCamera) { ViewportCamera = InCamera; }

    void Tick(float DeltaTime);

    void InitializeView(const FVector& InLocation, const FVector& InPivot);

    // Orbit
    void ResetView();
    void BeginOrbit(const FVector2& MousePos);
    void UpdateOrbit(const FVector2& MousePos);
    void EndOrbit();

    // Pan
    void BeginPan(const FVector2& MousePos);
    void UpdatePan(const FVector2& MousePos);
    void EndPan();

    // Zoom
    void Zoom(float Delta);

    bool IsOrbiting() const { return bOrbiting; }
    bool IsPanning() const { return bPanning; }

  private:
    void ApplyOrbitTransform();

  private:
    FViewportCamera* ViewportCamera = nullptr;

    FVector BestViewLocation = FVector::Zero();
    FVector BestViewPivot = FVector::Zero();

    // Reset
    float   StartYaw, StartPitch, StartRadius;
    float   TargetYaw, TargetPitch, TargetRadius;
    FVector StartPivot, TargetPivot;
    float   OrbitLerpAlpha = 0.0f;
    bool    bOrbitLerping = false;
    float   ResetLerpSpeed = 3.0f;

    // Orbit
    FVector OrbitPivot = FVector::Zero();
    float   CurrentYaw = 0.0f;
    float   CurrentPitch = 0.0f;
    float   OrbitRadius = 3.0f;
    float   OrbitSensitivity = 0.2f;

    FVector2 LastMousePos;
    bool     bOrbiting = false;

    // Pan
    bool     bPanning = false;
    FVector2 PanStartMouse;
    FVector  PanStartLocation;
    float    PanSensitivity = 0.01f;

    // Zoom
    float MinZoomRadius = 0.1f;
    float ZoomStep = 0.3f;
};
