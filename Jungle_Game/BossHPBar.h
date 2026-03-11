#pragma once

#include "GameObject.h"
#include "BossObject.h"

class UBossHPBar : public UGameObject
{
public:
    UBossHPBar();
    ~UBossHPBar() override;

public:
    void Update(float DeltaTime) override;
    void Render(URenderer& renderer) override;
    void SetBossObject(UBossObject* boss) { BossObj = boss; }

private:
    UBossObject* BossObj = nullptr;

    static constexpr float FullScaleX = 0.6f;
    static constexpr float VerticalOffset = -0.1f;
};
