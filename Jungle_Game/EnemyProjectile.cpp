#include "EnemyProjectile.h"
#include "GameObject.h"
#include "CircleCollider.h"
#include "SceneManager.h"
#include <functional>
#include <algorithm>

UEnemyProjectile::UEnemyProjectile()
{
    Initialize();
}

UEnemyProjectile::UEnemyProjectile(FTransform transform)
    : UGameObject(transform)
{
    Initialize();
}

UEnemyProjectile::~UEnemyProjectile()
{

}

void UEnemyProjectile::Initialize()
{
    Velocity = FVector(0, -1.0f, 0);

    SetTextureName("enemy_projectile");
    Collider->SetLayer(ECollisionLayer::EnemyProjectile);
    Collider->AddContactLayer(ECollisionLayer::Player);
    Collider->AddCollisionCallback(ECollisionEvent::Enter, std::bind(&UEnemyProjectile::OnCollisionEnter, this));
}

void UEnemyProjectile::Update(float DeltaTime)
{
    if (player)
    {
        if (bToPlayer) {
            TargetDirection = (player->GetTransform().Location - Transform.Location).Normalize();
        }

        //FVector targetDirection = (player->GetTransform().Location - Transform.Location).Normalize();
        float targetRotation = std::atan2f(TargetDirection.y, TargetDirection.x) - 3.14159265f / 2.0f;

        // 각도 차이 계산
        float angleDiff = targetRotation - Transform.Rotation.z;

        angleDiff = std::fmod(angleDiff + 3.14159265f, 2.0f * 3.14159265f);
        if (angleDiff < 0) angleDiff += 2.0f * 3.14159265f;
        angleDiff -= 3.14159265f;

        // 각속도 제한
        constexpr float rotationSpeed = 0.3f;
        float maxStep = rotationSpeed * DeltaTime;
        angleDiff = (std::max)(-maxStep, (std::min)(angleDiff, maxStep));

        Transform.Rotation.z += angleDiff;
    }

    constexpr float speed = 1.5f;
    Velocity.x = std::cosf(Transform.Rotation.z + 3.14159265f / 2.0f) * speed;
    Velocity.y = std::sinf(Transform.Rotation.z + 3.14159265f / 2.0f) * speed;
    Transform.Location = Transform.Location + Velocity * DeltaTime;
}

void UEnemyProjectile::Render(URenderer& renderer)
{
    FConstantBuffer data;
    data.position = Transform.Location;
    data.rotation = Transform.Rotation;
    data.scale = Transform.Scale;

    renderer.UpdateConstantBuffer(data);
    renderer.BindTexture(GetTextureName());
    renderer.Render();
}

bool UEnemyProjectile::CheckCollision(UGameObject* other)
{
    return false;
}

void UEnemyProjectile::ApplyImpulse(const FConstantBuffer& v)
{

}

void UEnemyProjectile::OnCollisionEnter()
{
    Destroy();
    //SetPendingDestroy(true); // same effect
}

void UEnemyProjectile::SetTargetDirection(FVector direction, FVector offset)
{
    direction.Normalize();
    TargetDirection = (direction + offset).Normalize();
    bToPlayer = false;
}
