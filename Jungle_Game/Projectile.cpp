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
    constexpr float speed = 0.1f;
    Velocity = FVector(0.0f, speed, 0.0f);

    Radius = 0.5f;
    Mass = 1.0f;

    SetTextureName("projectile");
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
    // 충돌 처리

    // 같은 발사체
    UProjectile* otherP = dynamic_cast<UProjectile*>(other);
    if (otherP == nullptr)
        return false;

    // 두 오브젝트 사이 거리 계산
    FConstantBuffer diff;
    diff.position.x = Transform.Location.x - otherP->Transform.Location.x;
    diff.position.y = Transform.Location.y - otherP->Transform.Location.y;
    diff.position.z = Transform.Location.z - otherP->Transform.Location.z;

    float distance = sqrtf(diff.position.x * diff.position.x + diff.position.y * diff.position.y + diff.position.z * diff.position.z);

    // 두 오브젝트의 반지름 합
    float radiusSum = Radius + otherP->Radius;

    return distance < radiusSum;


    // TODO: 다른 몬스터 등
}

void UProjectile::ApplyImpulse(const FConstantBuffer& v)
{

}
