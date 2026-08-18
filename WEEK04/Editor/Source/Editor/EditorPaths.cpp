#include "Editor/EditorPaths.h"
#include "Core/Misc/Paths.h"

std::filesystem::path FEditorPaths::AboutLogo()
{
    return FPaths::Combine(FPaths::AppRoot(), L"Content", L"Texture", L"Logo", L"copass.png");
}

std::filesystem::path FEditorPaths::DefaultSceneDirectory()
{
    return FPaths::Combine(FPaths::AppContentDir(), L"Scenes");
}

std::filesystem::path FEditorPaths::ConfigDirectory()
{
    return FPaths::Combine(FPaths::AppRoot(), L"Config");
}

std::filesystem::path FEditorPaths::EditorConfigFile()
{
    return FPaths::Combine(ConfigDirectory(), L"Editor.ini");
}

std::filesystem::path FEditorPaths::EditorStartupConfigFile()
{
    return FPaths::Combine(ConfigDirectory(), L"EditorStartup.ini");
}

std::filesystem::path FEditorPaths::ImGuiDefaultIniFile()
{
    return FPaths::Combine(ConfigDirectory(), L"imgui.default.ini");
}

std::filesystem::path FEditorPaths::ImGuiUserIniFile()
{
    return FPaths::Combine(ConfigDirectory(), L"imgui.user.ini");
}

