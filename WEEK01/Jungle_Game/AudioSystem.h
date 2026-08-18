#pragma once

#include <xaudio2.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <queue>

#pragma comment(lib, "xaudio2.lib")

struct FAudio
{
    WAVEFORMATEX wfx;
    std::vector<BYTE> AudioData;
    unsigned int audioBytes;
};

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
    VoiceCallback(std::queue<IXAudio2SourceVoice*>* pool) : VoicePool(pool) {}

    void OnBufferEnd(void* pBufferContext) override
    {
        if (pBufferContext && VoicePool)
        {
            IXAudio2SourceVoice* voice = static_cast<IXAudio2SourceVoice*>(pBufferContext);
            voice->Stop(0);
            voice->FlushSourceBuffers();
            VoicePool->push(voice);
        }
    }

    void OnStreamEnd() override {}
    void OnVoiceProcessingPassEnd() override {}
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) override {}
    void OnBufferStart(void* pBufferContext) override {}
    void OnLoopEnd(void* pBufferContext) override {}
    void OnVoiceError(void* pBufferContext, HRESULT Error) override {}

private:
    std::queue<IXAudio2SourceVoice*>* VoicePool;
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
    void InitializeSFXVoicePool(size_t poolSize = 16);
    IXAudio2SourceVoice* GetAvailableSFXVoice();

private:
    IXAudio2* XAudio2 = nullptr;
    IXAudio2MasteringVoice* MasteringVoice = nullptr;

    IXAudio2SourceVoice* BGMSourceVoice = nullptr;
    IXAudio2SubmixVoice* SFXSubmixVoice = nullptr;

	std::unordered_map<std::string, FAudio> SoundMap;

    std::queue<IXAudio2SourceVoice*> SFXVoicePool;
    std::vector<IXAudio2SourceVoice*> AllSFXVoices;
    VoiceCallback* Callback = nullptr;
};

