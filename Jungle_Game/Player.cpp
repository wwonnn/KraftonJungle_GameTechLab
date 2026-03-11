#include "Player.h"
#include "InputManager.h"
#include "AudioSystem.h"
#include "Projectile.h"
#include "CollisionSystem.h"
#include "SceneManager.h"
#include "GlobalState.h"
#include "ItemObject.h"
#include "ScoreManager.h"

UPlayer::UPlayer()
    :UGameObject()
{
    Initialize();
}

UPlayer::UPlayer(FTransform transform)
    :UGameObject(transform)
{
    Initialize();
}

UPlayer::~UPlayer()
{
}

void UPlayer::Initialize()
{
    Velocity = FVector(0.5f, 0.0f, 0.0f);
    Collider->SetLayer(ECollisionLayer::Player);
    Collider->AddContactLayer(ECollisionLayer::Enemy);
    Collider->AddContactLayer(ECollisionLayer::EnemyProjectile);
    Collider->AddContactLayer(ECollisionLayer::Item);
    Collider->AddCollisionCallback(ECollisionEvent::Enter, std::bind(&UPlayer::OnCollisionEnter, this, std::placeholders::_1));
    Collider->AddCollisionCallback(ECollisionEvent::Exit, std::bind(&UPlayer::OnCollisionExit, this));
}

void UPlayer::Update(float DeltaTime)
{
    UInputManager& Input = UInputManager::Get();

    if (Input.GetKey(VK_LEFT))
    {
        Transform.Location.x -= Velocity.x * DeltaTime;
    }
    if (Input.GetKey(VK_RIGHT))
    {
        Transform.Location.x += Velocity.x * DeltaTime;
    }

    if (ShootCooldownTimer > 0.0f)
    {
        ShootCooldownTimer -= DeltaTime;
    }

    if (Input.GetKeyDown(VK_SPACE))
    {
        FireProjectile();
    }

    if (bIsInvincible) {
        InvincibilityTime += DeltaTime;

        if (InvincibilityTime >= TotalInvincibleDuration) {
            bIsInvincible = false;
            InvincibilityTime = 0.0f;
        }
    }

    CheckWallCollision();
}

void UPlayer::FireProjectile()
{
    if (ShootCooldownTimer > 0.0f){ return; }

    UAudioSystem::Get().Play("shoot");

    const auto& currentStats = PowerUpTable[Power];
    ShootCooldownTimer = currentStats.ProjectileCooldown;

    for(int i = 0; i < currentStats.ShootCount; i++)
    {
        float xOffset = (i - (currentStats.ShootCount - 1) / 2.0f) * shootInterval;
        UProjectile* projectile = new UProjectile(
            FTransform(FVector(Transform.Location.x + xOffset, Transform.Location.y, Transform.Location.z), FVector(0, 0, 0), FVector(0.1f, 0.1f, 0.1f))
        );
        projectile->SetVelocity(currentStats.ProjectileVelocity);
        SceneManager::Get().GetcurrentScene()->CreateGameObject(projectile);
        projectile->Destroy(2.0f);
    }
}

void UPlayer::TakeDamage() {
    if (bIsDead || bIsInvincible) return;

    UAudioSystem::Get().Play("playerHit");

    HP--;
    if (OnHPChanged) {
        OnHPChanged(HP);
    }

    bIsInvincible = true;

    if (HP < 0) {
        HP = -1;
        bIsDead = true;
        GlobalState::Get().bAllStageCleared = false;
    }
}

void UPlayer::Heal()
{
    if (bIsDead) return;
    if (HP >= maxHP) {
        ScoreManager::Get().AddScore(100);
        return;
    }

    HP++;
    if (OnHPChanged) {
        OnHPChanged(HP);
    }
}

void UPlayer::PowerUp()
{
    if (Power >= maxPower || bIsDead) return;
    Power++;
}


void UPlayer::Render(URenderer& renderer)
{
    FConstantBuffer data;
    data.position = Transform.Location;
    data.rotation = Transform.Rotation;
    data.scale = Transform.Scale;

    FEffectConstantBuffer effectData;
    if (bIsInvincible) {
        effectData.time = InvincibilityTime;
    }

    renderer.UpdateConstantBuffer(data);
    renderer.UpdateEffectConstantBuffer(effectData);
    renderer.BindTexture(GetTextureName());
    renderer.Render();

    effectData.time = 0.0f;
    renderer.UpdateEffectConstantBuffer(effectData);
}

void UPlayer::CheckWallCollision()
{
    UCollisionSystem::Get().CheckWallCollision(Transform.Location, Transform.Scale);
}

void UPlayer::OnCollisionEnter(UCircleCollider* other)
{
    if (other->GetLayer() == ECollisionLayer::Item)
    {
        
        UItemObject* item = dynamic_cast<UItemObject*>(other->GetOwner());
        if (item->GetID() == EItemType::Heal) {
            Heal();
        }
        else if (item->GetID() == EItemType::Upgrade) {
            PowerUp();
        }
    }
    else if (other->GetLayer() == ECollisionLayer::Enemy || other->GetLayer() == ECollisionLayer::EnemyProjectile)
    {
        TakeDamage();
    }
}

void UPlayer::OnCollisionExit()
{
}
