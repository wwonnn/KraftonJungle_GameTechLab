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

    void OnCollisionEnter();
    void OnCollisionExit();

    bool IsDead() const { return bIsDead; }

    int GetHP();
    void TakeDamage();

    std::function<void(int)> OnHPChanged;

private:
    void CheckWallCollision();

private:
    FVector Velocity;

    bool bIsDead = false;
    UAudioSystem* AudioSystem = nullptr;

    int HP = 3;
    float delayTime = 3.0f;
    float HitTime = 3.0f;
};
