#pragma once

#include "Core/EngineAPI.h"

#include <filesystem>

class ENGINE_API FEditorPaths
{
  public:
    static std::filesystem::path AboutLogo();
    static std::filesystem::path DefaultSceneDirectory();
    static std::filesystem::path ConfigDirectory();
    static std::filesystem::path EditorConfigFile();
    static std::filesystem::path EditorStartupConfigFile();
    static std::filesystem::path ImGuiDefaultIniFile();
    static std::filesystem::path ImGuiUserIniFile();
};
