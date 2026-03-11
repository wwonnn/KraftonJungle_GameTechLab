#include "ItemObject.h"
#include "ImGuiManager.h"

UItemObject::UItemObject()
    :UGameObject()
{
    Initialize();
}

UItemObject::UItemObject(FTransform transform)
    :UGameObject(transform)
{
    Initialize();
}

UItemObject::~UItemObject()
{
}

void UItemObject::Update(float DeltaTime)
{
    Transform.Location = Transform.Location + Velocity * DeltaTime;
}

void UItemObject::Render(URenderer& renderer)
{
    FConstantBuffer data;
    data.position = Transform.Location;
    data.rotation = Transform.Rotation;
    data.scale = Transform.Scale;

    renderer.UpdateConstantBuffer(data);
    renderer.BindTexture(GetTextureName());
    renderer.Render();
}

void UItemObject::Initialize() {
    Velocity = FVector(0.0f, -0.4f, 0.0f);
    SetTextureName("heart");
    Collider->SetLayer(ECollisionLayer::Item);
    Collider->AddContactLayer(ECollisionLayer::Player);
    Collider->AddCollisionCallback(ECollisionEvent::Enter, std::bind(&UItemObject::OnCollisionEnter, this, std::placeholders::_1));

    Destroy(4.0f);
}

void UItemObject::OnCollisionEnter(UCircleCollider* other)
{
    if (other->GetLayer() == ECollisionLayer::Player) {
        Destroy();
    }
}
