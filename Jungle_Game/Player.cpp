#include "Player.h"
#include "InputManager.h"
#include "AudioSystem.h"
#include "Projectile.h"
#include "CollisionSystem.h"
#include "SceneManager.h"
#include "GlobalState.h"

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

    ShootCooldownTimer = ShootCooldown;
    UAudioSystem::Get().Play("shoot");
    UGameObject* projectile = new UProjectile(
        FTransform(Transform.Location, FVector(0, 0, 0), FVector(0.1f, 0.1f, 0.1f))
    );

    SceneManager::Get().GetcurrentScene()->CreateGameObject(projectile);
    projectile->Destroy(1.5f);
}

int UPlayer::GetHP() { return HP; }


void UPlayer::TakeDamage() {
    if (bIsDead || bIsInvincible) return;

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
    if (bIsDead || HP >= 2) return;

    HP++;
    if (OnHPChanged) {
        OnHPChanged(HP);
    }
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
        Heal();
    }
    else if (other->GetLayer() == ECollisionLayer::Enemy || other->GetLayer() == ECollisionLayer::EnemyProjectile)
    {
        TakeDamage();
    }
}

void UPlayer::OnCollisionExit()
{
}
