#pragma once

#include <string>

struct GlobalData
{
    float BGMVolume = 0.5f;
    float SFXVolume = 1.0f;
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

public:
    GlobalData& GetData() { return Data; }
    void SetGlobalData(const GlobalData& data);

private:
    const std::string SaveFilePath = "settings.dat";

    GlobalData Data;
};

