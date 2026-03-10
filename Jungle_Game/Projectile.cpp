#pragma once

#include "Projectile.h"

UProjectile::UProjectile()
{
    Initialize();
}

UProjectile::UProjectile(FTransform transform)
    : UGameObject(transform)
{
    Initialize();
}

UProjectile::~UProjectile()
{

}

void UProjectile::Initialize()
{
    Velocity = FVector(0.0f, 1.0f, 0.0f);
    SetTextureName("projectile");
    Collider->SetLayer(ECollisionLayer::Projectile);
    Collider->AddContactLayer(ECollisionLayer::Enemy);
    Collider->AddCollisionCallback(ECollisionEvent::Enter, std::bind(&UProjectile::OnCollisionEnter, this));
}

void UProjectile::Update(float DeltaTime)
{
    Transform.Location = Transform.Location + Velocity * DeltaTime;
}

void UProjectile::Render(URenderer& renderer)
{
    FConstantBuffer data;
    data.position = Transform.Location;
    data.rotation = Transform.Rotation;
    data.scale = Transform.Scale;

    renderer.UpdateConstantBuffer(data);
    renderer.BindTexture(GetTextureName());
    renderer.Render();
}

bool UProjectile::CheckCollision(UGameObject* other)
{
    return false;
}

void UProjectile::ApplyImpulse(const FConstantBuffer& v)
{

}

void UProjectile::OnCollisionEnter()
{
    Destroy();
}
