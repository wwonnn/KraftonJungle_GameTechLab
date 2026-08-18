#pragma once

#include "Renderer.h"
#include "Math.h"
#include "CircleCollider.h"
#include "Animator.h"

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
    virtual void Destroy();
    virtual void Destroy(float delay);
    void TickDestroyTimer(float deltaTime);

    std::string GetTextureName() const { return TextureName; }
    void SetTextureName(const std::string& name) { TextureName = name; }

    FTransform GetTransform() const { return Transform; }
    UCircleCollider* GetCollider() const { return Collider; }

    bool IsPendingDestroy() const { return bIsPendingDestroy; }

protected:
    FTransform Transform;
    UCircleCollider* Collider = nullptr;
    bool bIsPendingDestroy = false;
    float DestroyDelayTimer = -1.0f;

private:
    std::string TextureName = "player";
};
