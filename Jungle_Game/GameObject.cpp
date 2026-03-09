#include "GameObject.h"
#include <cmath>

std::vector<UGameObject*> UGameObject::GameObjectList;

UGameObject::UGameObject()
{
	// 오브젝트 리스트에 자신을 등록
	ListIndex = GameObjectList.size();
	GameObjectList.push_back(this);
}

UGameObject::~UGameObject()
{
	// 오브젝트 리스트에서 자신을 제거
	GameObjectList[ListIndex] = GameObjectList.back();
	GameObjectList.pop_back();
}

UProjectile::UProjectile()
{
	Location.x = 0.0f;
	Location.x = 0.0f;
	Location.x = 0.0f;

	Velocity.x = 0.0f;
	Velocity.y = 0.0f;
	Velocity.z = 0.0f;

	Radius = 1.0f;
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
    FConstantBuffer constants;
    constants.position = Location;
    constants.scale = FVector(1.0f, 1.0f, 1.0f);
	renderer.UpdateConstantBuffer(constants);
}

bool UProjectile::CheckCollision(UGameObject* other)
{
	// 충돌 처리

	// 같은 발사체
	UProjectile* otherP = dynamic_cast< UProjectile*>(other);
	if (otherP == nullptr)
		return false;

	// 두 오브젝트 사이 거리 계산
	FVector diff;
	diff.x = Location.x - otherP->Location.x;
	diff.y = Location.y - otherP->Location.y;
	diff.z = Location.z - otherP->Location.z;

	float distance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

	// 두 오브젝트의 반지름 합
	float radiusSum = Radius + otherP->Radius;

	return distance < radiusSum;


	// TODO: 다른 몬스터 등
}

void UProjectile::ApplyImpulse(const FConstantBuffer& v)
{

}
