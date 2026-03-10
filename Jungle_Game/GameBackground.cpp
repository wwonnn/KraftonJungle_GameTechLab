#include "GameBackground.h"

UGameBackground::UGameBackground()
    : UGameObject()
{
    SetTextureName("game_bg");
}

UGameBackground::UGameBackground(FTransform transform)
    : UGameObject(transform)
{
    SetTextureName("game_bg");
}

UGameBackground::~UGameBackground()
{
}

void UGameBackground::Update(float DeltaTime)
{
    Transform.Location.y -= ScrollSpeed * DeltaTime;
    if (Transform.Location.y <= -2.0f)
    {
        Transform.Location.y = 0.0f;
    }
}

void UGameBackground::Render(URenderer& renderer)
{
    FConstantBuffer constants;
    constants.position = Transform.Location;
    constants.rotation = Transform.Rotation;
    constants.scale = Transform.Scale;
    renderer.UpdateConstantBuffer(constants);
    renderer.BindTexture(GetTextureName());
    renderer.Render();

    constants.position = Transform.Location + FVector(0.0f, 2.0f, 0.0f);
    renderer.UpdateConstantBuffer(constants);
    renderer.BindTexture(GetTextureName());
    renderer.Render();
}
