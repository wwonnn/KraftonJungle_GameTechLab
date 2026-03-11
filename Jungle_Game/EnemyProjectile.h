#pragma once

#include "Renderer.h"
#include "GameObject.h"
#include "Player.h"

class UEnemyProjectile : public UGameObject
{
public:
    UEnemyProjectile();
    UEnemyProjectile(FTransform transform);
    ~UEnemyProjectile() override;

public:
    virtual void Update(float DeltaTime) override;
    virtual void Render(URenderer& renderer) override;

    void OnCollisionEnter();
    void SetPlayer(UPlayer* player) { this->player = player; }
    void SetTargetDirection(FVector direction, FVector offset = { 0.0f, 0.0f });

private:
    void Initialize();

private:
    FVector TargetDirection;
    FVector Velocity;
    UPlayer* player = nullptr;
    bool bToPlayer = true; // 기본 플레이어 향해 발사
};
