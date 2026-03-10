#include "Player.h"
#include "InputManager.h"
#include "CollisionSystem.h"

UPlayer::UPlayer()
    :UGameObject()
{
    Initialize();
}

UPlayer::UPlayer(FTransform transform)
    :UGameObject(transform)
{
    Initialize();
}

UPlayer::~UPlayer()
{

}

void UPlayer::Initialize()
{
    Velocity = FVector(0.5f, 0.0f, 0.0f);
    Collider->SetLayer(ECollisionLayer::Player);
    Collider->AddContactLayer(ECollisionLayer::Enemy);
    Collider->AddCollisionCallback(ECollisionEvent::Enter, std::bind(&UPlayer::OnCollisionEnter, this));
    Collider->AddCollisionCallback(ECollisionEvent::Exit, std::bind(&UPlayer::OnCollisionExit, this));
}

void UPlayer::Update(float DeltaTime)
{
    UInputManager& Input = UInputManager::Get();

    if (Input.GetKey(VK_LEFT))
    {
        Transform.Location.x -= Velocity.x * DeltaTime;
    }
    if (Input.GetKey(VK_RIGHT))
    {
        Transform.Location.x += Velocity.x * DeltaTime;
    }
/*    if (Input.GetKey(VK_UP))
    {
        Location.y += Speed * DeltaTime;
    }
    if (Input.GetKey(VK_DOWN))
    {
        Location.y -= Speed * DeltaTime;
    }*/

    CheckWallCollision();
}

void UPlayer::Render(URenderer& renderer)
{
    FConstantBuffer data;
    data.position = Transform.Location;
    data.rotation = Transform.Rotation;
    data.scale = Transform.Scale;

    renderer.UpdateConstantBuffer(data);
    renderer.BindTexture(GetTextureName());
    renderer.Render();
}

bool UPlayer::CheckCollision(UGameObject* other)
{
    // 충돌 처리
    return false;
}

void UPlayer::CheckWallCollision()
{
    UCollisionSystem::Get().CheckWallCollision(Transform.Location, Transform.Scale);
}

void UPlayer::ApplyImpulse(const FConstantBuffer& v)
{

}

void UPlayer::OnCollisionEnter()
{
    OutputDebugString(L"Player enter\n");
}

void UPlayer::OnCollisionExit()
{
    OutputDebugString(L"Player exit\n");
}
