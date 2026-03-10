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
    ~UEnemyObject() override;

public:
    void Update(float DeltaTime) override;
    void Render(URenderer& renderer) override;
    bool CheckCollision(UGameObject* other) override { return true; }
    void ApplyImpulse(const FConstantBuffer& v) override {}

    void SetPlayer(UPlayer* player) { Player = player; }
    void SetMovementStrategy(std::unique_ptr<IMovementStrategy> strategy);
    IMovementStrategy* GetMovementStrategy() const;
    void TransitionToState(EnemyState newState);

private:
    void Initialize();
    void Spawn(float DeltaTime);
    void Move(float DeltaTime);
    void Dead(UCircleCollider* other);

private:
    bool isSpawned = false;
    float spawnTimer = 0.0f;
    FTransform SpawnedTransform;
    FVector StartPos;
    FVector TargetPos;
    EnemyState State = EnemyState::Spawn;

    std::unique_ptr<IMovementStrategy> MovementStrategy;

    UPlayer* Player = nullptr;

    UAnimator* Animator = nullptr;
};

