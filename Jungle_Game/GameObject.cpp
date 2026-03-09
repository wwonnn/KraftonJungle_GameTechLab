#include "GameObject.h"
#include "InputManager.h"
#include <cmath>

std::vector<UGameObject*> UGameObject::GameObjectList;

UGameObject::UGameObject()
{
	// 오브젝트 리스트에 자신을 등록
	ListIndex = GameObjectList.size();
	GameObjectList.push_back(this);

    Transform = FTransform();
    Collider = new UCircleCollider(this);
}

UGameObject::UGameObject(FTransform transform)
    : Transform(transform)
{
    // 오브젝트 리스트에 자신을 등록
    ListIndex = GameObjectList.size();
    GameObjectList.push_back(this);

    Collider = new UCircleCollider(this);
}

UGameObject::~UGameObject()
{
	// 오브젝트 리스트에서 자신을 제거
	GameObjectList[ListIndex] = GameObjectList.back();
	GameObjectList.pop_back();
}

UProjectile::UProjectile()
{
	Velocity.x = 0.0f;
	Velocity.y = 0.0f;
	Velocity.z = 0.0f;

	Radius = 0.5f;
	Mass = 1.0f;
}

UProjectile::~UProjectile()
{

}

void UProjectile::Update(float DeltaTime)
{
}

void UProjectile::Render(URenderer& renderer)
{
    FConstantBuffer data;
    data.position = Transform.Location;
    data.scale = Transform.Scale;

    renderer.UpdateConstantBuffer(data);
    renderer.Render();
}

bool UProjectile::CheckCollision(UGameObject* other)
{
	// 충돌 처리

	// 같은 발사체
	UProjectile* otherP = dynamic_cast< UProjectile*>(other);
	if (otherP == nullptr)
		return false;

	// 두 오브젝트 사이 거리 계산
	FConstantBuffer diff;
    diff.position.x = Transform.Location.x - otherP->Transform.Location.x;
    diff.position.y = Transform.Location.y - otherP->Transform.Location.y;
    diff.position.z = Transform.Location.z - otherP->Transform.Location.z;

	float distance = sqrtf(diff.position.x * diff.position.x + diff.position.y * diff.position.y + diff.position.z * diff.position.z);

	// 두 오브젝트의 반지름 합
	float radiusSum = Radius + otherP->Radius;

	return distance < radiusSum;


	// TODO: 다른 몬스터 등
}

void UProjectile::ApplyImpulse(const FConstantBuffer& v)
{

}


