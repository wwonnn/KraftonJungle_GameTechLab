#pragma once

#include "GameObject.h"
#include "Player.h"
#include "MovementStrategies.h"
#include "MovementSequence.h"
#include <memory>

// 상태 패턴
enum class EnemyState
{
    Spawn,
    Move,
    Dead
};

class UEnemyObject : public UGameObject
{
public:
    UEnemyObject();
    UEnemyObject(FTransform transform);
    virtual ~UEnemyObject();

public:
    virtual void Initialize();
    virtual void Update(float DeltaTime) override;
    virtual void Render(URenderer& renderer) override;
    bool CheckCollision(UGameObject* other) override { return true; }
    void ApplyImpulse(const FConstantBuffer& v) override {}

    void SetPlayer(UPlayer* player) { Player = player; }
    UPlayer* GetPlayer() const { return Player; }
    void SetMovementStrategy(std::unique_ptr<IMovementStrategy> strategy);
    IMovementStrategy* GetMovementStrategy() const;
    void TransitionToState(EnemyState newState);

    bool IsShootable() const { return bShootable; }
    void SetShootable(bool shootable) { bShootable = shootable; }

protected:
    void Spawn(float DeltaTime);
    void Move(float DeltaTime);
    virtual void Dead(UCircleCollider* other);
    void FireEnemyProjectile();

protected:
    bool isSpawned = false;
    float spawnTimer = 0.0f;
    FTransform SpawnedTransform;
    FVector StartPos;
    FVector TargetPos;
    EnemyState State = EnemyState::Spawn;

    bool bShootable = true;

    std::unique_ptr<IMovementStrategy> MovementStrategy;

    UPlayer* Player = nullptr;

    UAnimator* Animator = nullptr;
};

