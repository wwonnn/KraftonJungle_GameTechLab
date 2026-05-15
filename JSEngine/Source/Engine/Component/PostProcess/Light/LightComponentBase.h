#pragma once
#include "Component/SceneComponent.h"

class ULightComponentBase : public USceneComponent {
public:
	DECLARE_CLASS(ULightComponentBase, USceneComponent)
	ULightComponentBase() = default;
    virtual void PostDuplicate(UObject* Original) override;
	virtual void Serialize(FArchive& Ar) override;

protected:
	~ULightComponentBase() = default;

public:
    UPROPERTY(EditAnywhere, DisplayName="Color", Speed=0.1, Animatable)
    FColor LightColor = FColor::White();
	UPROPERTY(EditAnywhere, DisplayName="Intensity", Speed=0.1, Animatable)
	float Intensity = 1.0f;

	UPROPERTY(EditAnywhere, DisplayName="Cast Shadows")
	bool bCastShadows = true;
};
