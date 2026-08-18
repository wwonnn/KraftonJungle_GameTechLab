#include "GlobalSettings.h"
#include "AudioSystem.h"
#include "nlohmann/json.hpp"
#include <algorithm>
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

            if (j.contains("Scoreboard") && j["Scoreboard"].is_array())
            {
                for (const auto& entry : j["Scoreboard"])
                {
                    if (entry.contains("Username") && entry.contains("Score"))
                    {
                        Data.Scoreboard.push_back({ entry["Username"].get<std::string>(), entry["Score"].get<int>() });
                    }
                }
            }

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
    j["Scoreboard"] = nlohmann::json::array();
    for (const auto& entry : Data.Scoreboard)
    {
        j["Scoreboard"].push_back({ {"Username", entry.Username}, {"Score", entry.Score} });
    }

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

void GlobalSettings::AddScore(const std::string& username, int score)
{
    Data.Scoreboard.push_back({ username, score });
    std::sort(Data.Scoreboard.begin(), Data.Scoreboard.end());
    Data.Scoreboard.resize(std::min<int>(Data.Scoreboard.size(), static_cast<size_t>(10)));
    Save();
}
