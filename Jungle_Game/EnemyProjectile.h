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

private:
    void Initialize();

private:
    FVector Velocity;
    UPlayer* player = nullptr;
};
