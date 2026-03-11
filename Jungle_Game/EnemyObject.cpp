#include "EnemyObject.h"
#include "EventSystem.h"
#include "ScoreManager.h"
#include "GameObject.h"
#include "EnemyProjectile.h"
#include "SceneManager.h"
#include "Random.h"
#include "ItemObject.h"

UEnemyObject::UEnemyObject()
    : UGameObject()
{
}

UEnemyObject::UEnemyObject(FTransform transform)
    : UGameObject(transform)
{
}

UEnemyObject::~UEnemyObject()
{
    if (Animator)
    {
        delete Animator;
        Animator = nullptr;
    }
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

    Animator->Update(DeltaTime, Animator->State);
}

void UEnemyObject::Render(URenderer& renderer)
{
    FConstantBuffer constants;
    constants.position = Transform.Location;
    constants.rotation = Transform.Rotation;
    constants.scale = Transform.Scale;

    FSpriteUVBuffer spriteConstants;
    if (State != EnemyState::Dead)
        spriteConstants = { 0.0f, 0.0f, 1.0f, 1.0f };
    else
        Animator->GetFrameUV(spriteConstants.UVOffsetX, spriteConstants.UVOffsetY, spriteConstants.UVScaleX, spriteConstants.UVScaleY);

    renderer.UpdateConstantBuffer(constants);
    renderer.UpdateSpriteUV(spriteConstants);
    renderer.BindTexture(GetTextureName());
    renderer.Render();

    spriteConstants = { 0.0f, 0.0f, 1.0f, 1.0f };
    renderer.UpdateSpriteUV(spriteConstants);
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
    Animator = new UAnimator();

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

    // 발사체 발사
    if (IsShootable() && Transform.Location.y > -0.2f) {
        constexpr float chancePerSecond = 0.2f;
        if (Random::Range(0.0f, 1.0f) < chancePerSecond * deltaTime) {
            FireEnemyProjectile();
        }
    }
}

void UEnemyObject::FireEnemyProjectile()
{
    UGameObject* enemyProjectile = SceneManager::Get().GetcurrentScene()->CreateGameObject(new UEnemyProjectile(
        FTransform(Transform.Location, GetTransform().Rotation, FVector(0.1f, 0.1f, 0.1f))));
    dynamic_cast<UEnemyProjectile*>(enemyProjectile)->SetPlayer(GetPlayer());
    enemyProjectile->Destroy(2.0f);
}

void UEnemyObject::Dead(UCircleCollider* other)
{
    if (State == EnemyState::Dead)
        return;

    SetTextureName("deadAnim");
    auto anim = FAnimationClip();
    anim.TextureName = GetTextureName();
    anim.FrameCount = 3;
    anim.Columns = 3;
    anim.Rows = 1;
    anim.FrameDuration = 0.1f;
    anim.bLoop = false;

    Animator->SetAnimationClip(anim);

    TransitionToState(EnemyState::Dead);
    if (other->GetLayer() == ECollisionLayer::Projectile)
    {
        // ignore collision with player, only projectile can kill enemy
        ScoreManager::Get().AddScore(100);

        float random = rand() % 100;
        if (random < 30) {
           UGameObject* itemobj = new UItemObject(FTransform(Transform.Location), 0);
           SceneManager::Get().GetcurrentScene()->CreateGameObject(itemobj);
        }
    }
    UAudioSystem::Get().Play("pop");
    Destroy(0.3f);
    EventSystem::Get().Trigger("EnemyDied");
}
