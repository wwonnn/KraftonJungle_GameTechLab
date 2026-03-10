#include "IntroBackgroundObject.h"

IntroBackgroundObject::IntroBackgroundObject()
    : UGameObject()
{
    SetTextureName("background");
}

IntroBackgroundObject::IntroBackgroundObject(FTransform transform)
    : UGameObject(transform)
{
    SetTextureName("background");
}

IntroBackgroundObject::~IntroBackgroundObject()
{
}

void IntroBackgroundObject::Update(float DeltaTime)
{
}

void IntroBackgroundObject::Render(URenderer& renderer)
{
    FConstantBuffer constants;
    constants.position = Transform.Location;
    constants.rotation = Transform.Rotation;
    constants.scale = Transform.Scale;

    FSpriteUVBuffer spriteConstants = { 0.0f, 0.0f, 1.0f, 1.0f };
    renderer.UpdateConstantBuffer(constants);
    renderer.UpdateSpriteUV(spriteConstants);
    renderer.BindTexture(GetTextureName());
    renderer.Render();
}
