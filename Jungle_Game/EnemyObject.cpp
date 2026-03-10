#include "EnemyObject.h"
#include "EventSystem.h"

UEnemyObject::UEnemyObject()
    : UGameObject()
{
    Initialize();
}

UEnemyObject::UEnemyObject(FTransform transform)
    : UGameObject(transform)
{
    Initialize();
}

UEnemyObject::~UEnemyObject()
{
}

void UEnemyObject::Update(float DeltaTime)
{
    switch (State)
    {
    case EnemyState::Spawn:
        Spawn(DeltaTime);
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

void UEnemyObject::Initialize()
{
    SetTextureName("enemy");

    Collider->SetLayer(ECollisionLayer::Enemy);
    Collider->AddContactLayer(ECollisionLayer::Player);
    Collider->AddContactLayer(ECollisionLayer::Projectile);
    Collider->AddCollisionCallback(ECollisionEvent::Enter, std::bind(&UEnemyObject::Dead, this, std::placeholders::_1));

    SpawnedTransform = Transform;
    Transform.Location.x = rand() % 100 / 100.0f * 2.0f - 1.0f; // -1.0 ~ 1.0 
    Transform.Location.y = rand() % 100 / 100.0f * 1.3f - 0.3f; // -0.3 ~ 1.0 

    StartPos = Transform.Location;
    TargetPos = SpawnedTransform.Location;
}

void UEnemyObject::Spawn(float deltaTime)
{
    float speed = 1.0f;

    spawnTimer += speed * deltaTime;
    if (spawnTimer >= 1.0f)
        spawnTimer = 1.0f;

    Transform.Location = StartPos + (TargetPos - StartPos) * spawnTimer;
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
    EventSystem::Get().Trigger("EnemyDied");
}
