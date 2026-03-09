#pragma once

#include "Renderer.h"
#include "Math.h"

// STL
#include <vector>

struct FTransform
{
    FVector Location;
    FVector Rotation;
    FVector Scale;

    FTransform(FVector location = { 0, 0, 0 }, FVector rotation = { 0, 0, 0 }, FVector scale = { 1.0f, 1.0f, 1.0f })
        : Location(location),
        Rotation(rotation),
        Scale(scale)
    {}
};

class UGameObject 
{
public:
	UGameObject();
    UGameObject(FTransform transform);
	virtual ~UGameObject();

public:
	virtual void Update(float DeltaTime) = 0;
	virtual void Render(URenderer& renderer) = 0;
	virtual bool CheckCollision(UGameObject* other) = 0;
	virtual void ApplyImpulse(const FConstantBuffer& v) = 0;

	// 현재 생성된 게임 오브젝트 리스트
	static std::vector<UGameObject*> GameObjectList;

protected:
    FTransform Transform;

private:
	int ListIndex;
};

// Test Object (발사체)
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
	FVector Velocity;
	float Radius;
	float Mass;
};
