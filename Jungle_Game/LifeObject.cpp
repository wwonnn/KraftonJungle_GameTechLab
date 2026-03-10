#include "LifeObject.h"

LifeObject::LifeObject()
    : UGameObject()
{
    SetTextureName("player");
}

LifeObject::LifeObject(FTransform transform)
    : UGameObject(transform)
{
    SetTextureName("player");
}

LifeObject::~LifeObject()
{
}

void LifeObject::Update(float DeltaTime)
{
}

void LifeObject::Render(URenderer& renderer)
{
    FConstantBuffer constants;
    constants.position = Transform.Location;
    constants.rotation = Transform.Rotation;
    constants.scale = Transform.Scale;
    renderer.UpdateConstantBuffer(constants);
    renderer.BindTexture(GetTextureName());
    renderer.Render();
}
