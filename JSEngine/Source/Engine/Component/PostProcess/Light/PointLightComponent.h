#pragma once
#include "LightComponent.h"

class UPointLightComponent : public ULightComponent
{
public:
    DECLARE_CLASS(UPointLightComponent, ULightComponent)
    virtual void PostDuplicate(UObject* Original) override;
	virtual void Serialize(FArchive& Ar) override;

protected:
	virtual FMatrix ComputePerspectiveShadowMatrix(const FMatrix& CamView, const FMatrix& CamProj,
		const TArray<FBoundingBox>* VisibleObjectsBounds) const override;

	//virtual void PrintShadowMapDebugInfo(TArray<FPropertyDescriptor>& OutProps) const override;

public:
    UPROPERTY(EditAnywhere, DisplayName="Attenuation Radius", SerializeName="AttenuationRadius", Speed=0.1, Animatable)
    float AttenuationRadius		= 10.f;
    UPROPERTY(EditAnywhere, DisplayName="Light Falloff", SerializeName="LightFalloffExponent", Speed=0.1, Animatable)
    float LightFalloffExponent	= 1.f;
};
