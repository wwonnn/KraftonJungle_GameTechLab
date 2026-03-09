#include "EnemyObject.h"

UEnemyObject::UEnemyObject()
{
    Transform.Scale = FVector(0.3f, 0.3f, 0.3f);
    SetTextureName("enemy");
}

UEnemyObject::~UEnemyObject()
{
}

void UEnemyObject::Update(float DeltaTime)
{
}

void UEnemyObject::Render(URenderer& renderer)
{
    FConstantBuffer constants;
    constants.position = Transform.Location;
    constants.rotation = Transform.Rotation;
    constants.scale = Transform.Scale;
    renderer.UpdateConstantBuffer(constants);
    renderer.BindTexture(GetTextureName());
    renderer.Render();
}
