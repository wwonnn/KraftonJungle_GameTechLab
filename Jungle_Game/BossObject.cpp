#include "BossObject.h"
#include "EventSystem.h"
#include "ScoreManager.h"

UBossObject::UBossObject()
    : UEnemyObject()
{
}

UBossObject::UBossObject(FTransform transform)
    : UEnemyObject(transform)
{
}

UBossObject::~UBossObject()
{
}

void UBossObject::Update(float DeltaTime)
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

    if (bIsHurt) {
        HurtTime += DeltaTime;

        if (HurtTime >= TotalHurtEffectDuration) {
            bIsHurt = false;
            HurtTime = 0.0f;
            SetTextureName("boss");
        }
    }
}

void UBossObject::Initialize()
{
    SetTextureName("boss");
    Animator = new UAnimator();

    Collider->SetLayer(ECollisionLayer::Enemy);
    Collider->AddContactLayer(ECollisionLayer::Player);
    Collider->AddContactLayer(ECollisionLayer::Projectile);
    Collider->AddCollisionCallback(ECollisionEvent::Enter, std::bind(&UBossObject::Hurt, this, std::placeholders::_1));

    SpawnedTransform = Transform;
    Transform.Location.x = 0.0f; 
    Transform.Location.y = 1.0f; 

    StartPos = Transform.Location;
    TargetPos = SpawnedTransform.Location;

    if (Collider != nullptr)
    {
        Collider->SetRadius(0.5f);
    }
}

void UBossObject::Render(URenderer& renderer)
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

    FEffectConstantBuffer effectData;
    if (bIsHurt) {
        effectData.time = HurtTime;
    }

    renderer.UpdateConstantBuffer(constants);
    renderer.UpdateSpriteUV(spriteConstants);
    renderer.UpdateEffectConstantBuffer(effectData);
    renderer.BindTexture(GetTextureName());
    renderer.Render();

    spriteConstants = { 0.0f, 0.0f, 1.0f, 1.0f };
    renderer.UpdateSpriteUV(spriteConstants);

    effectData.time = 0.0f;
    renderer.UpdateEffectConstantBuffer(effectData);
}

void UBossObject::Hurt(UCircleCollider* other)
{
    if (other->GetLayer() == ECollisionLayer::Projectile)
    {
        bIsHurt = true;
        SetTextureName("bossHurt");
        HP -= 1.0f;
        if(HP <= 0.0f)
        {
            Dead(other);
        }
    }
}

void UBossObject::Dead(UCircleCollider* other)
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
    ScoreManager::Get().AddScore(1000);

    UAudioSystem::Get().Play("pop");
    Destroy(0.3f);
    EventSystem::Get().Trigger("EnemyDied");
}
