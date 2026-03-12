#include "ItemObject.h"
#include "ImGuiManager.h"
#include "ItemDatabase.h"
#include "ImGuiManager.h"
#include "Random.h"

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

UItemObject::UItemObject(FTransform transform, EItemType itemtype)
    :UGameObject(transform)
{
    Itemtype = itemtype;
    const auto& data = UItemDatabase::Get().GetItem(itemtype);
    Transform.Scale = data.Scale;
    Velocity = data.Veclocity;
    SetTextureName(data.TextureName);
    Destroy(data.DestroyDelay);

    auto strategy = CreateStrategy(data);
    MovementStrategy = std::move(strategy);

    Initialize();
}


UItemObject::~UItemObject()
{
}

void UItemObject::Update(float DeltaTime)
{
    if (MovementStrategy)
    {
        FVector newPos;
        float newRot;
        if (MovementStrategy->Update(newPos, newRot, Transform.Location, FVector(0, 0, 0), DeltaTime))
        {
            // 이동 완료
        }
        else
        {
            // 이동 진행 중
            Transform.Location = newPos;
            Transform.Rotation.z = newRot;
        }
    }
    // Transform.Location = Transform.Location + Velocity * DeltaTime;
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

std::unique_ptr<IMovementStrategy> UItemObject::CreateStrategy(const FItemTemplate& data) {
    if (data.MovementStrategyType == "ItemZigZag")
        return std::make_unique<ItemZigZagMovement>(FVector(0, 1, 0), data.Veclocity.y,0.3f, 1.0f);
    else if (data.MovementStrategyType == "ItemRandom")
        return std::make_unique<ItemLinearMovement>(FVector(Random::Range(-0.5f, 0.5f), Random::Range(-1.0f, 1.0f), 0), Random::Range(0.4f, 0.7f));
    return nullptr;
}
