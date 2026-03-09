#include "Player.h"
#include "InputManager.h"

UPlayer::UPlayer()
    :UGameObject()
{
}

UPlayer::UPlayer(FTransform transform)
    :UGameObject(transform)
{
}

UPlayer::~UPlayer()
{

}

void UPlayer::Update(float DeltaTime)
{
    // 이동 입력
    const float Speed = 0.1f;
    UInputManager& Input = UInputManager::Get();

    if (Input.GetKey(VK_LEFT))
    {
        Transform.Location.x -= Speed * DeltaTime;
    }
    if (Input.GetKey(VK_RIGHT))
    {
        Transform.Location.x += Speed * DeltaTime;
    }
/*    if (Input.GetKey(VK_UP))
    {
        Location.y += Speed * DeltaTime;
    }
    if (Input.GetKey(VK_DOWN))
    {
        Location.y -= Speed * DeltaTime;
    }*/
}

void UPlayer::Render(URenderer& renderer)
{
    renderer.UpdateConstantBuffer({ Transform.Location, Transform.Scale });
    renderer.Render();
}

bool UPlayer::CheckCollision(UGameObject* other)
{
    // 충돌 처리
    return false;
}

void UPlayer::ApplyImpulse(const FConstantBuffer& v)
{

}
