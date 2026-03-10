#pragma once

#include "Renderer.h"
#include "GameObject.h"
#include "AudioSystem.h"

class UPlayer : public UGameObject
{
public:
    UPlayer();
    UPlayer(FTransform transform);
    virtual ~UPlayer();

public:
    void Initialize();
    virtual void Update(float deltaTime) override;
    virtual void Render(URenderer& renderer) override;
    virtual bool CheckCollision(UGameObject* other) override;
    virtual void ApplyImpulse(const FConstantBuffer& v) override;

    void OnCollisionEnter();
    void OnCollisionExit();

    void SetAudioSystem(UAudioSystem* sys);

private:
    void CheckWallCollision();

private:
    FVector Velocity;
    UAudioSystem* AudioSystem = nullptr;
};
