#include "EnemyObject.h"

UEnemyObject::UEnemyObject()
    : UGameObject()
{
    SetTextureName("enemy");
}

UEnemyObject::UEnemyObject(FTransform transform)
    : UGameObject(transform)
{
    SetTextureName("enemy");

    Collider->SetLayer(ECollisionLayer::Enemy);
    Collider->AddContactLayer(ECollisionLayer::Player);
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
