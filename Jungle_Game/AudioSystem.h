#pragma once

#include <xaudio2.h>
#include <unordered_map>
#include <string>
#include <vector>

#pragma comment(lib, "xaudio2.lib")

struct FAudio
{
    WAVEFORMATEX wfx;
    std::vector<BYTE> AudioData;
    unsigned int audioBytes;
};

class UAudioSystem
{
public:
    static UAudioSystem& Get()
    {
        static UAudioSystem instance;
        return instance;
    }

	UAudioSystem() = default;
	~UAudioSystem() = default;

public:
	bool Create();
	void Release();

	void LoadFromFile(const std::string& filePath, const std::string& soundName);
	void Play(const std::string& soundName);

    void PlayBGM(const std::string& soundName);
    void StopBGM();

    void SetBGMVolume(float volume);
    void SetSFXVolume(float volume);

private:
    IXAudio2* XAudio2 = nullptr;
    IXAudio2MasteringVoice* MasteringVoice = nullptr;

    IXAudio2SourceVoice* BGMSourceVoice = nullptr;
    IXAudio2SubmixVoice* SFXSubmixVoice = nullptr;

	std::unordered_map<std::string, FAudio> SoundMap;
};

