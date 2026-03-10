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
    renderer.UpdateConstantBuffer(constants);
    renderer.BindTexture(GetTextureName());
    renderer.Render();
}
