#pragma once

#include <string>
#include <vector>

struct UserData
{
    std::string Username;
    int Score;

    bool operator<(const UserData& other) const
    {
        return Score > other.Score;
    }
};

struct GlobalData
{
    float BGMVolume = 0.5f;
    float SFXVolume = 1.0f;

    std::vector<UserData> Scoreboard;
};

class GlobalSettings
{
public:
    static GlobalSettings& Get()
    {
        static GlobalSettings instance;
        return instance;
    }
    GlobalSettings() = default;
    ~GlobalSettings() = default;

public:
    void Load();
    void Save();

    GlobalData& GetData() { return Data; }
    void SetGlobalData(const GlobalData& data);
    void AddScore(const std::string& username, int score);

private:
    const std::string SaveFilePath = "settings.dat";

    GlobalData Data;
};

