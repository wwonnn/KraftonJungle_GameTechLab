#pragma once

#include "Renderer.h"

// STL
#include <vector>

class UGameObject 
{
public:
	UGameObject();
	virtual ~UGameObject();

public:
	virtual void Update(float DeltaTime) = 0;
	virtual void Render(URenderer& renderer) = 0;
	virtual bool CheckCollision(UGameObject* other) = 0;
	virtual void ApplyImpulse(const FConstantBuffer& v) = 0;

	// 현재 생성된 게임 오브젝트 리스트
	static std::vector<UGameObject*> GameObjectList;

private:
	int ListIndex;
};

class UProjectile : public UGameObject
{
public:
	UProjectile();
	virtual ~UProjectile();

public:
	virtual void Update(float DeltaTime) override;
	virtual void Render(URenderer& renderer) override;
	virtual bool CheckCollision(UGameObject* other) override;
	virtual void ApplyImpulse(const FConstantBuffer& v) override;

private:
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
};
