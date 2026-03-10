#include "GlobalSettings.h"
#include "AudioSystem.h"
#include "nlohmann/json.hpp"

#include <fstream>

void GlobalSettings::Load()
{
    std::ifstream file(SaveFilePath);
    if (file.is_open())
    {
        try
        {
            nlohmann::json j;
            file >> j;

            Data.BGMVolume = j.value("BGMVolume", 0.5f);
            Data.SFXVolume = j.value("SFXVolume", 1.0f);

            file.close();
        }
        catch (const nlohmann::json::exception& e)
        {
            Data.BGMVolume = 0.5f;
            Data.SFXVolume = 1.0f;
        }
    }
    else
    {
        Save();
    }
}

void GlobalSettings::Save()
{
    nlohmann::json j;

    j["BGMVolume"] = Data.BGMVolume;
    j["SFXVolume"] = Data.SFXVolume;

    std::ofstream file(SaveFilePath);
    if (file.is_open())
    {
        file << j.dump(4);
        file.close();
    }
}

void GlobalSettings::SetGlobalData(const GlobalData& data)
{
    Data = data;
    Save();
    UAudioSystem::Get().SetBGMVolume(Data.BGMVolume);
    UAudioSystem::Get().SetSFXVolume(Data.SFXVolume);
}
