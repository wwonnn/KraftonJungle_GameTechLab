#pragma once

#include "GameObject.h"
#include "Player.h"
#include "EnemyObject.h"
#include "MovementStrategies.h"
#include "MovementSequence.h"
#include <memory>

class UBossObject : public UEnemyObject
{
public:
    UBossObject();
    UBossObject(FTransform transform);
    virtual ~UBossObject() override;

public:
    void Update(float DeltaTime) override;
    void Initialize() override;
    void Render(URenderer& renderer) override;

private:
    void Hurt(UCircleCollider* other);
    void Dead(UCircleCollider* other) override;
    void BossAttack(float deltaTime);
    void FireBossProjectile();

private:
    float HP = 30.0f;
    bool bIsHurt = false;
    float HurtTime = 0.0f;
    float TotalHurtEffectDuration = 1.0f;
};
