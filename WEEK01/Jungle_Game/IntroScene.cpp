#include "IntroScene.h"
#include "IntroBackgroundObject.h"
#include "EventSystem.h"
#include "SceneManager.h"
#include "GlobalSettings.h"

void IntroScene::Initialize() {
    // Load resources, initialize variables, etc.
    UGameObject* background = CreateGameObject(new IntroBackgroundObject(
        FTransform(FVector(), FVector(), FVector(2.0f, 2.0f, 1.0f))));
    UGameObject* title = CreateGameObject(new IntroBackgroundObject(
        FTransform(FVector(0.0f, 0.5f, 0.0f), FVector(), FVector(1.0f, 0.5f, 1.0f))));
    title->SetTextureName("title");
}

void IntroScene::Update(float deltaTime) {
    if (State == "MainMenu")
    {
        UpdateMainMenu();
    }
    else if (State == "Credits")
    {
        UpdateCredits();
    }
    else if (State == "Settings")
    {
        UpdateSettings();
    }
}

void IntroScene::Render() {
    Renderer->BeginFrame();

    for (UGameObject* const& obj : GameObjects) {
        obj->Render(*Renderer);
    }

    if (State == "MainMenu")
    {
        RenderMainMenu();
    }
    else if (State == "Credits")
    {
        RenderCredits();
    }
    else if (State == "Settings")
    {
        RenderSettings();
    }
}

void IntroScene::Release() {
    UScene::Release();
}

void IntroScene::UpdateMainMenu()
{
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

        switch (OptionIndex) {
        case 0:
            // Start the game
            SceneManager::Get().ChangeScene(SceneType::InGame);
            break;
        case 1:
            SceneManager::Get().ChangeScene(SceneType::Infinite);
            break;
        case 2:
            // Show credits
            State = "Credits";
            break;
        case 3:
            State = "Settings";
            break;
        case 4:
            // Exit the game
            PostQuitMessage(0);
            break;
        }
    }
}

void IntroScene::RenderMainMenu()
{
    int option = 0;
    Renderer->DrawString(L"START!", 400.0f, 600.0f, option == OptionIndex ? FVector(0.0f, 1.0f, 0.0f) : FVector(1.0f, 1.0f, 1.0f));
    option++;
    Renderer->DrawString(L"INFINITE MODE", 400.0f, 640.0f, option == OptionIndex ? FVector(0.0f, 1.0f, 0.0f) : FVector(1.0f, 1.0f, 1.0f));
    option++;
    Renderer->DrawString(L"CREDIT", 400.0f, 720.0f, option == OptionIndex ? FVector(0.0f, 1.0f, 0.0f) : FVector(1.0f, 1.0f, 1.0f));
    option++;
    Renderer->DrawString(L"SETTING", 400.0f, 760.0f, option == OptionIndex ? FVector(0.0f, 1.0f, 0.0f) : FVector(1.0f, 1.0f, 1.0f));
    option++;
    Renderer->DrawString(L"EXIT", 400.0f, 820.0f, option == OptionIndex ? FVector(0.0f, 1.0f, 0.0f) : FVector(1.0f, 1.0f, 1.0f));
}

void IntroScene::UpdateCredits()
{
    UInputManager& input = UInputManager::Get();

    if (input.GetKeyDown(VK_SPACE)) {
        UAudioSystem::Get().Play("choose");
        State = "MainMenu";
    }
}

void IntroScene::RenderCredits()
{
    Renderer->DrawString(L"Team 6", 400.0f, 500.0f, FVector(1.0f, 1.0f, 1.0f));
    Renderer->DrawString(L"Gihong Kim", 400.0f, 600.0f, FVector(1.0f, 1.0f, 1.0f));
    Renderer->DrawString(L"Junhyeok Oh", 400.0f, 650.0f, FVector(1.0f, 1.0f, 1.0f));
    Renderer->DrawString(L"Wonhee Seong", 400.0f, 700.0f, FVector(1.0f, 1.0f, 1.0f));
    Renderer->DrawString(L"Dongjin Kuk", 400.0f, 750.0f, FVector(1.0f, 1.0f, 1.0f));

    Renderer->DrawString(L"Press SPACE to return", 400.0f, 850.0f, FVector(1.0f, 1.0f, 0.0f));
}

void IntroScene::UpdateSettings()
{
    UInputManager& input = UInputManager::Get();

    if (input.GetKeyDown(VK_UP))
    {
        SettingOptionIndex = (SettingOptionIndex - 1 + MaxsettingOptions + 1) % (MaxsettingOptions + 1);
        UAudioSystem::Get().Play("select");
    }
    else if (input.GetKeyDown(VK_DOWN))
    {
        SettingOptionIndex = (SettingOptionIndex + 1) % (MaxsettingOptions + 1);
        UAudioSystem::Get().Play("select");
    }
    else if (input.GetKeyDown(VK_SPACE)) {
        UAudioSystem::Get().Play("choose");
        State = "MainMenu";
    }

    if (input.GetKeyDown(VK_LEFT))
    {
        if (SettingOptionIndex == 0)
        {
            float newVolume = std::max<float>(0.0f, GlobalSettings::Get().GetData().BGMVolume - 0.1f);
            GlobalSettings::Get().GetData().BGMVolume = newVolume;

            GlobalData newData = GlobalSettings::Get().GetData();
            newData.BGMVolume = newVolume;

            GlobalSettings::Get().SetGlobalData(newData);
        }
        else if (SettingOptionIndex == 1)
        {
            float newVolume = std::max<float>(0.0f, GlobalSettings::Get().GetData().SFXVolume - 0.1f);
            GlobalSettings::Get().GetData().SFXVolume = newVolume;

            GlobalData newData = GlobalSettings::Get().GetData();
            newData.SFXVolume = newVolume;

            GlobalSettings::Get().SetGlobalData(newData);
        }

        UAudioSystem::Get().Play("select");
    }
    else if (input.GetKeyDown(VK_RIGHT))
    {
        if (SettingOptionIndex == 0)
        {
            float newVolume = std::min<float>(1.0f, GlobalSettings::Get().GetData().BGMVolume + 0.1f);
            GlobalSettings::Get().GetData().BGMVolume = newVolume;

            GlobalData newData = GlobalSettings::Get().GetData();
            newData.BGMVolume = newVolume;

            GlobalSettings::Get().SetGlobalData(newData);
        }
        else if (SettingOptionIndex == 1)
        {
            float newVolume = std::min<float>(1.0f, GlobalSettings::Get().GetData().SFXVolume + 0.1f);
            GlobalSettings::Get().GetData().SFXVolume = newVolume;

            GlobalData newData = GlobalSettings::Get().GetData();
            newData.SFXVolume = newVolume;

            GlobalSettings::Get().SetGlobalData(newData);
        }

        UAudioSystem::Get().Play("select");
    }
}

void IntroScene::RenderSettings()
{
    int bgmPercent = static_cast<int>((GlobalSettings::Get().GetData().BGMVolume * 100 + 5) / 10) * 10;
    int sfxPercent = static_cast<int>((GlobalSettings::Get().GetData().SFXVolume * 100 + 5) / 10) * 10;

    std::wstring bgmText = L"BGM Volume: " + std::to_wstring(bgmPercent) + L"%";
    std::wstring sfxText = L"SFX Volume: " + std::to_wstring(sfxPercent) + L"%";

    int option = 0;
    Renderer->DrawString(bgmText.c_str(), 400.0f, 500.0f, option == SettingOptionIndex ? FVector(0.0f, 1.0f, 0.0f) : FVector(1.0f, 1.0f, 1.0f));
    option++;
    Renderer->DrawString(sfxText.c_str(), 400.0f, 600.0f, option == SettingOptionIndex ? FVector(0.0f, 1.0f, 0.0f) : FVector(1.0f, 1.0f, 1.0f));

    Renderer->DrawString(L"Press SPACE to return", 400.0f, 850.0f, FVector(1.0f, 1.0f, 0.0f));
}
