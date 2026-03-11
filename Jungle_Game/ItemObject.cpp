#include "ItemObject.h"
#include "ImGuiManager.h"
#include "ItemDatabase.h"
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

UItemObject::UItemObject(FTransform transform, int itemID)
    :UGameObject(transform)
{
    ItemID = itemID;
    const auto& data = UItemDatabase::Get().GetItem(itemID);
    Transform.Scale = data.Scale;
    Velocity = data.Veclocity;
    SetTextureName(data.TextureName);
    Destroy(data.DestroyDelay);

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

    Collider->SetLayer(ECollisionLayer::Item);
    Collider->AddContactLayer(ECollisionLayer::Player);
    Collider->AddCollisionCallback(ECollisionEvent::Enter, std::bind(&UItemObject::OnCollisionEnter, this, std::placeholders::_1));
}

void UItemObject::OnCollisionEnter(UCircleCollider* other)
{
    if (other->GetLayer() == ECollisionLayer::Player) {
        UAudioSystem::Get().Play("heal");
        Destroy();
    }
}
