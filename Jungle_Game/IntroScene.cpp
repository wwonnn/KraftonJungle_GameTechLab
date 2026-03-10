#include "IntroScene.h"
#include "IntroBackgroundObject.h"
#include "EventSystem.h"
#include "SceneManager.h"

void IntroScene::Initialize() {
    // Load resources, initialize variables, etc.
    UGameObject* background = CreateGameObject(new IntroBackgroundObject(
        FTransform(FVector(), FVector(), FVector(2.0f, 2.0f, 1.0f))));
    UGameObject* title = CreateGameObject(new IntroBackgroundObject(
        FTransform(FVector(0.0f, 0.5f, 0.0f), FVector(), FVector(1.0f, 0.5f, 1.0f))));
    title->SetTextureName("title");
}

void IntroScene::Update(float deltaTime) {
    UInputManager& input = UInputManager::Get();

    if (input.GetKeyDown(VK_UP)) {
        OptionIndex = (OptionIndex - 1 + MaxOptions + 1) % (MaxOptions + 1);
        UAudioSystem::Get().Play("select");
    }
    else if (input.GetKeyDown(VK_DOWN)) {
        OptionIndex = (OptionIndex + 1) % (MaxOptions + 1);
        UAudioSystem::Get().Play("select");
    }
     else if (input.GetKeyDown(VK_SPACE)) {
        UAudioSystem::Get().Play("choose");

        if (IsCredit)
        {
            IsCredit = false;
            OptionIndex = 0;
        }
        else
        {
            switch (OptionIndex) {
            case 0:
                // Start the game
                SceneManager::Get().ChangeScene(SceneType::InGame);
                break;
            case 1:
                // Show credits
                IsCredit = true;
                break;
            case 2:
                // Exit the game
                PostQuitMessage(0);
                break;
            }
        }
    }
}

void IntroScene::Render() {
    Renderer->BeginFrame();

    for (UGameObject* const& obj : GameObjects) {
        obj->Render(*Renderer);
    }

    if (IsCredit)
    {
        Renderer->DrawString(L"Team 6", 400.0f, 500.0f, FVector(1.0f, 1.0f, 1.0f));
        Renderer->DrawString(L"Name", 400.0f, 600.0f, FVector(1.0f, 1.0f, 1.0f));
        Renderer->DrawString(L"Name", 400.0f, 650.0f, FVector(1.0f, 1.0f, 1.0f));
        Renderer->DrawString(L"Name", 400.0f, 700.0f, FVector(1.0f, 1.0f, 1.0f));
        Renderer->DrawString(L"Dongjin Kuk", 400.0f, 750.0f, FVector(1.0f, 1.0f, 1.0f));

        Renderer->DrawString(L"Press SPACE to return", 400.0f, 850.0f, FVector(0.0f, 1.0f, 0.0f));
    }
    else
    {
        int option = 0;
        Renderer->DrawString(L"START!", 400.0f, 600.0f, option == OptionIndex ? FVector(0.0f, 1.0f, 0.0f) : FVector(1.0f, 1.0f, 1.0f));
        option++;
        Renderer->DrawString(L"CREDIT", 400.0f, 680.0f, option == OptionIndex ? FVector(0.0f, 1.0f, 0.0f) : FVector(1.0f, 1.0f, 1.0f));
        option++;
        Renderer->DrawString(L"EXIT", 400.0f, 760.0f, option == OptionIndex ? FVector(0.0f, 1.0f, 0.0f) : FVector(1.0f, 1.0f, 1.0f));
    }

    Renderer->SwapBuffer();
}

void IntroScene::Release() {
    for (UGameObject* obj : GameObjects) {
        delete obj;
    }
}
