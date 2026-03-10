#include "EnemyObject.h"

UEnemyObject::UEnemyObject()
    : UGameObject()
{
    SetTextureName("enemy");

    Collider->SetLayer(ECollisionLayer::Enemy);
    Collider->AddContactLayer(ECollisionLayer::Player);
    Collider->AddContactLayer(ECollisionLayer::Projectile);
    Collider->AddCollisionCallback(ECollisionEvent::Enter, std::bind(&UEnemyObject::Dead, this, std::placeholders::_1));
}

UEnemyObject::UEnemyObject(FTransform transform)
    : UGameObject(transform)
{
    SetTextureName("enemy");

    Collider->SetLayer(ECollisionLayer::Enemy);
    Collider->AddContactLayer(ECollisionLayer::Player);
    Collider->AddContactLayer(ECollisionLayer::Projectile);
    Collider->AddCollisionCallback(ECollisionEvent::Enter, std::bind(&UEnemyObject::Dead, this, std::placeholders::_1));
}

UEnemyObject::~UEnemyObject()
{
}

void UEnemyObject::Update(float DeltaTime)
{
    switch (State)
    {
    case EnemyState::Spawn:
        //TransitionToState(EnemyState::Move);
        break;
    case EnemyState::Move:
        Move(DeltaTime);
        break;
    case EnemyState::Dead:
        break;
    }
}

void UEnemyObject::Render(URenderer& renderer)
{
    FConstantBuffer constants;
    constants.position = Transform.Location;
    constants.rotation = Transform.Rotation;
    constants.scale = Transform.Scale;

    renderer.UpdateConstantBuffer(constants);
    renderer.BindTexture(GetTextureName());
    renderer.Render();
}

/// <summary>
/// 몬스터 스폰 시 이동 패턴 설정
/// </summary>
/// <param name="strategy"></param>
void UEnemyObject::SetMovementStrategy(std::unique_ptr<IMovementStrategy> strategy)
{
    MovementStrategy = std::move(strategy);
    if (MovementStrategy) MovementStrategy->Reset();
}

IMovementStrategy* UEnemyObject::GetMovementStrategy() const
{
    return MovementStrategy.get();
}

void UEnemyObject::TransitionToState(EnemyState newState)
{
    State = newState;
}

void UEnemyObject::Move(float deltaTime)
{
    if (MovementStrategy)
    {
        FVector newPos;
        float newRot;
        if (MovementStrategy->Update(newPos, newRot, Transform.Location, Player ? Player->GetTransform().Location : FVector(), deltaTime))
        {
            // 이동 완료

            // TODO: 플레이어 총에 맞아 사망
            TransitionToState(EnemyState::Dead);
        }
        else
        {
            // 이동 진행 중
            Transform.Location = newPos;
            Transform.Rotation.z = newRot;
        }
    }
}

void UEnemyObject::Dead(UCircleCollider* other)
{
    Destroy();
}
