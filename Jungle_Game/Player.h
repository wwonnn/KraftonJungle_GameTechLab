#pragma once

#include "Renderer.h"
#include "GameObject.h"
#include "AudioSystem.h"
#include <functional>

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
    virtual bool CheckCollision(UGameObject* other) override;
    virtual void ApplyImpulse(const FConstantBuffer& v) override;

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

    int HP = 2;
    int maxHP = 2;
    int Power = 1;
    int maxPower = 3;
    float shootInterval = 0.03f;

    bool bIsInvincible = false;
    float InvincibilityTime = 0.0f;
    float TotalInvincibleDuration = 1.0f;

    static constexpr float ShootCooldown = 0.2f;
    float ShootCooldownTimer = 0.0f;
};
