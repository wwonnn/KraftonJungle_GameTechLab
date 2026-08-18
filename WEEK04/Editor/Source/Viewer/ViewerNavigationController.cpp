#include "ViewerNavigationController.h"

void FViewerNavigationController::Tick(float DeltaTime)
{
    if (!ViewportCamera)
        return;

    if (bOrbitLerping)
    {
        OrbitLerpAlpha += DeltaTime * ResetLerpSpeed;
        float Alpha = FMath::Clamp(OrbitLerpAlpha, 0.0f, 1.0f);

        // 오빗 파라미터 보간
        CurrentYaw = FMath::LerpAngle(StartYaw, TargetYaw, Alpha);
        CurrentPitch = std::lerp(StartPitch, TargetPitch, Alpha);
        OrbitRadius = std::lerp(StartRadius, TargetRadius, Alpha);
        OrbitPivot = FVector::Lerp(StartPivot, TargetPivot, Alpha);

        ApplyOrbitTransform();

        if (Alpha >= 1.0f)
        {
            bOrbitLerping = false;
        }
    }
}

void FViewerNavigationController::InitializeView(const FVector& InLocation, const FVector& InPivot)
{
    if (!ViewportCamera)
        return;

// 베스트 뷰 저장
    BestViewLocation = InLocation;
    BestViewPivot = InPivot;

    // 현재 뷰 상태 초기화
    bPanning = false;
    PanStartMouse = FVector2::Zero();
    PanStartLocation = InLocation;

    // 오빗 파라미터도 베스트 뷰 기준으로 초기화
    OrbitPivot = InPivot;
    FVector Offset = InLocation - InPivot;
    OrbitRadius = Offset.Size();
    CurrentYaw = std::atan2(Offset.Y, Offset.X) * 180.0f / FMath::PI;
    CurrentPitch = std::asin(Offset.Z / OrbitRadius) * 180.0f / FMath::PI;

    ApplyOrbitTransform();
}

void FViewerNavigationController::ResetView()
{
    if (!ViewportCamera)
        return;

    StartYaw = CurrentYaw;
    StartPitch = CurrentPitch;
    StartRadius = OrbitRadius;
    StartPivot = OrbitPivot;

    TargetPivot = BestViewPivot;
    FVector Offset = BestViewLocation - BestViewPivot;
    TargetRadius = Offset.Size();
    TargetYaw = std::atan2(Offset.Y, Offset.X) * 180.0f / FMath::PI;
    TargetPitch = std::asin(Offset.Z / TargetRadius) * 180.0f / FMath::PI;

    OrbitLerpAlpha = 0.0f;
    bOrbitLerping = true;

    bPanning = false;
    PanStartMouse = FVector2::Zero();
    PanStartLocation = BestViewLocation;
}

void FViewerNavigationController::BeginOrbit(const FVector2& MousePos)
{
    bOrbiting = true;
    LastMousePos = MousePos;
}

void FViewerNavigationController::UpdateOrbit(const FVector2& MousePos)
{
    if (!bOrbiting || !ViewportCamera)
        return;

    float DeltaX = MousePos.X - LastMousePos.X;
    float DeltaY = MousePos.Y - LastMousePos.Y;
    LastMousePos = MousePos; // 현재 좌표를 저장해서 다음 프레임의 기준점으로 사용

    CurrentYaw += DeltaX * OrbitSensitivity;
    CurrentPitch = FMath::Clamp(CurrentPitch + DeltaY * OrbitSensitivity, -89.0f, 89.0f);

    ApplyOrbitTransform();
}

void FViewerNavigationController::EndOrbit() { bOrbiting = false; }

void FViewerNavigationController::BeginPan(const FVector2& MousePos)
{
    if (!ViewportCamera)
        return;

    bPanning = true;
    LastMousePos = MousePos;
}

void FViewerNavigationController::UpdatePan(const FVector2& MousePos)
{
    if (!bPanning || !ViewportCamera)
        return;

    float DeltaX = MousePos.X - LastMousePos.X;
    float DeltaY = MousePos.Y - LastMousePos.Y;
    LastMousePos = MousePos; 

    FVector Right = ViewportCamera->GetRightVector();
    FVector Up = ViewportCamera->GetUpVector();

    // PanSensitivity에 OrbitRadius를 곱해주면 거리감에 따른 속도가 자연스러워집니다.
    float   DynamicSensitivity = PanSensitivity * (OrbitRadius * 0.1f);
    FVector PanDelta = (Right * -DeltaX * DynamicSensitivity) + (Up * DeltaY * DynamicSensitivity);

    // 4. 핵심: 카메라와 피봇을 동시에 이동!
    OrbitPivot += PanDelta;
    ViewportCamera->SetLocation(ViewportCamera->GetLocation() + PanDelta);

}

void FViewerNavigationController::EndPan() { bPanning = false; }

void FViewerNavigationController::Zoom(float Delta)
{
    if (!ViewportCamera)
    {
        return;
    }

    float ZoomFactor = Delta >= 0.0f ? ZoomStep : -ZoomStep;

    OrbitRadius -= OrbitRadius * ZoomFactor;

    OrbitRadius = std::max(OrbitRadius, MinZoomRadius);

    ApplyOrbitTransform();

}

void FViewerNavigationController::ApplyOrbitTransform() 
{
    if (!ViewportCamera)
        return;

    float RadYaw = FMath::DegreesToRadians(CurrentYaw);
    float RadPitch = FMath::DegreesToRadians(CurrentPitch);

    FVector NewLocation;
    float   CosPitch = std::cos(RadPitch);

    NewLocation.X = OrbitPivot.X + OrbitRadius * CosPitch * std::cos(RadYaw);
    NewLocation.Y = OrbitPivot.Y + OrbitRadius * CosPitch * std::sin(RadYaw);
    NewLocation.Z = OrbitPivot.Z + OrbitRadius * std::sin(RadPitch);

    ViewportCamera->SetLocation(NewLocation);

    FVector Forward = (OrbitPivot - NewLocation).GetSafeNormal();

    FVector WorldUp = FVector::UpVector;
    if (FMath::Abs(FVector::DotProduct(Forward, WorldUp)) > 0.99f)
    {
        WorldUp = FVector(0.0f, 1.0f, 0.0f);
    }

    FVector Right = FVector::CrossProduct(WorldUp, Forward).GetSafeNormal();
    FVector Up = FVector::CrossProduct(Forward, Right);

    FMatrix RotationMatrix = FMatrix::Identity;
    RotationMatrix.SetAxes(Forward, Right, Up);

    FQuat NewQuat(RotationMatrix);
    NewQuat.Normalize();

    ViewportCamera->SetRotation(NewQuat);
}
