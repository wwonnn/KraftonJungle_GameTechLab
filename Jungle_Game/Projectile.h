#pragma once

#include "Renderer.h"
#include "GameObject.h"

class UProjectile : public UGameObject
{
public:
    UProjectile();
    UProjectile(FTransform transform);
    ~UProjectile() override;

public:
    void Initialize();
    virtual void Update(float DeltaTime) override;
    virtual void Render(URenderer& renderer) override;
    virtual bool CheckCollision(UGameObject* other) override;
    virtual void ApplyImpulse(const FConstantBuffer& v) override;

    void OnCollisionEnter();

private:
    FVector Velocity;
};
