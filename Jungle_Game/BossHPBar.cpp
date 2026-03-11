#include "BossHPBar.h"

UBossHPBar::UBossHPBar()
    : UGameObject()
{
    SetTextureName("boss_hp_bar");
    Transform.Location = FVector(0.0f, 0.95f, 0.0f);
    Transform.Scale = FVector(FullScaleX, 0.02f, 1.0f);
}

UBossHPBar::~UBossHPBar()
{
}

void UBossHPBar::Update(float DeltaTime)
{
    if (BossObj == nullptr)
    {
        return;
    }

    if (BossObj->IsPendingDestroy())
    {
        Destroy();
        return;
    }

    const FTransform bossTransform = BossObj->GetTransform();
    Transform.Scale.x = FullScaleX * BossObj->GetHPRatio();

    const float leftX = bossTransform.Location.x - (FullScaleX * 0.5f);
    Transform.Location.x = leftX + (Transform.Scale.x * 0.5f);
    Transform.Location.y = bossTransform.Location.y + bossTransform.Scale.y + VerticalOffset;
}

void UBossHPBar::Render(URenderer& renderer)
{
    FConstantBuffer constants;
    constants.position = Transform.Location;
    constants.rotation = Transform.Rotation;
    constants.scale = Transform.Scale;
    renderer.UpdateConstantBuffer(constants);
    renderer.BindTexture(GetTextureName());
    renderer.Render();
}
