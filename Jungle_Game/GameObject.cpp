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


