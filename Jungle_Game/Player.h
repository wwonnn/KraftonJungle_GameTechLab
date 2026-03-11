#pragma once

#include "Renderer.h"
#include "GameObject.h"
#include "AudioSystem.h"
#include <functional>
#include <unordered_map>

struct FPowerUpEffect {
    int ShootCount;
    FVector ProjectileVelocity;
    float ProjectileCooldown;
};

class UPlayer : public UGameObject
{
public:
    UPlayer();
    UPlayer(FTransform transform);
    virtual ~UPlayer() override;

public:
    void Initialize();
    virtual void Update(float deltaTime) override;
    virtual void Render(URenderer& renderer) override;

    void OnCollisionEnter(UCircleCollider* other);
    void OnCollisionExit();

    bool IsDead() const { return bIsDead; }
    void SetDead(bool dead) { bIsDead = dead; }

    void FireProjectile();
    int GetHP();
    void TakeDamage();
    void Heal();
    void PowerUp();

    std::function<void(int)> OnHPChanged;

private:
    void CheckWallCollision();

private:
    FVector Velocity;

    bool bIsDead = false;
    UAudioSystem* AudioSystem = nullptr;

    std::unordered_map<int, FPowerUpEffect> PowerUpTable = {
        {1, {1, { 0.0f, 1.0f, 0.0f } , 0.2f} },
        {2, { 2, { 0.0f, 1.0f, 0.0f } , 0.2f}},
        {3, { 2, { 0.0f, 1.0f, 0.0f } , 0.15f}},
        {4, { 2, { 0.0f, 1.2f, 0.0f } , 0.15f}},
        {5, { 3, { 0.0f, 1.2f, 0.0f } , 0.15f}},
        {6, { 3, { 0.0f, 1.2f, 0.0f } , 0.1f}},
        {7, { 3, { 0.0f, 1.5f, 0.0f } , 0.1f}},
    };

    int HP = 2;
    int maxHP = 2;
    int Power = 1;
    int maxPower = 7;
    float shootInterval = 0.03f;

    bool bIsInvincible = false;
    float InvincibilityTime = 0.0f;
    float TotalInvincibleDuration = 1.0f;

    float ShootCooldownTimer = 0.0f;
};
