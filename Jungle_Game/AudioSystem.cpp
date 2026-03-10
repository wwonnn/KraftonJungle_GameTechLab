#define DR_MP3_IMPLEMENTATION

#include "AudioSystem.h"
#include "dr_mp3.h"
#include "GlobalSettings.h"

bool UAudioSystem::Create()
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    XAudio2Create(&XAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    HRESULT hr = XAudio2->CreateMasteringVoice(&MasteringVoice);

    if (FAILED(hr))
    {
        MasteringVoice = nullptr;
    }

    hr = XAudio2->CreateSubmixVoice(&SFXSubmixVoice, 2, 44100);
    if (FAILED(hr))
    {
        SFXSubmixVoice = nullptr;
    }

    SetBGMVolume(GlobalSettings::Get().GetData().BGMVolume);
    SetSFXVolume(GlobalSettings::Get().GetData().SFXVolume);

    return true;
}

void UAudioSystem::Release()
{
    if (MasteringVoice)
    {
        MasteringVoice->DestroyVoice();
        MasteringVoice = nullptr;
    }

	if (XAudio2)
	{
        XAudio2->Release();
        XAudio2 = nullptr;
	}
}

void UAudioSystem::LoadFromFile(const std::string& filePath, const std::string& soundName)
{
    WAVEFORMATEX wfx = { 0 };

    drmp3_config config;
    drmp3_uint64 totalPCMFrameCount;
    short* sampleData = drmp3_open_file_and_read_pcm_frames_s16(filePath.c_str(), &config, &totalPCMFrameCount, nullptr);

    if (sampleData == NULL)
    {
        return;
    }

    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = config.channels;
    wfx.nSamplesPerSec = config.sampleRate;

    XAUDIO2_BUFFER buffer = { 0 };
    buffer.AudioBytes = totalPCMFrameCount * config.channels * sizeof(short);
    buffer.pAudioData = (BYTE*)sampleData;
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    FAudio newAudio;
    newAudio.wfx = { 0 };
    newAudio.wfx.wFormatTag = WAVE_FORMAT_PCM;
    newAudio.wfx.nChannels = config.channels;
    newAudio.wfx.nSamplesPerSec = config.sampleRate;
    newAudio.wfx.wBitsPerSample = 16;
    newAudio.wfx.nBlockAlign = (newAudio.wfx.nChannels * newAudio.wfx.wBitsPerSample) / 8;
    newAudio.wfx.nAvgBytesPerSec = newAudio.wfx.nSamplesPerSec * newAudio.wfx.nBlockAlign;
    newAudio.AudioData.assign(buffer.pAudioData, buffer.pAudioData + buffer.AudioBytes);
    newAudio.audioBytes = buffer.AudioBytes;

    SoundMap[soundName] = newAudio;
}

void UAudioSystem::Play(const std::string& soundName)
{
    if (MasteringVoice == nullptr)
    {
        if (FAILED(XAudio2->CreateMasteringVoice(&MasteringVoice)))
        {
            MasteringVoice = nullptr;
            return;
        }
    }
    if (SFXSubmixVoice == nullptr)
    {
        if (FAILED(XAudio2->CreateSubmixVoice(&SFXSubmixVoice, 2, 44100)))
        {
            SFXSubmixVoice = nullptr;
            return;
        }
    }

	auto iter = SoundMap.find(soundName);
	if (iter != SoundMap.end())
	{
        XAUDIO2_SEND_DESCRIPTOR sendDesc = { 0, SFXSubmixVoice };
        XAUDIO2_VOICE_SENDS sendList = { 1, &sendDesc };

        IXAudio2SourceVoice* sourceVoice;
        if (FAILED(XAudio2->CreateSourceVoice(&sourceVoice, &iter->second.wfx, 0, 2.0f, nullptr, &sendList)))
        {
            return;
        }

        XAUDIO2_BUFFER buffer = { 0 };
        buffer.pAudioData = iter->second.AudioData.data();
        buffer.AudioBytes = iter->second.audioBytes;
        buffer.Flags = XAUDIO2_END_OF_STREAM;

        sourceVoice->SubmitSourceBuffer(&buffer);
        sourceVoice->Start();
	}
}

void UAudioSystem::PlayBGM(const std::string& soundName)
{
    if (BGMSourceVoice)
    {
        BGMSourceVoice->Stop();
        BGMSourceVoice->DestroyVoice();
        BGMSourceVoice = nullptr;
    }

    auto iter = SoundMap.find(soundName);
    if (iter != SoundMap.end())
    {
        if (FAILED(XAudio2->CreateSourceVoice(&BGMSourceVoice, &iter->second.wfx)))
        {
            return;
        }

        XAUDIO2_BUFFER buffer = { 0 };
        buffer.pAudioData = iter->second.AudioData.data();
        buffer.AudioBytes = iter->second.audioBytes;
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

        BGMSourceVoice->SubmitSourceBuffer(&buffer);
        BGMSourceVoice->SetVolume(GlobalSettings::Get().GetData().BGMVolume);
        BGMSourceVoice->Start();
    }
}

void UAudioSystem::StopBGM()
{
    if (BGMSourceVoice)
    {
        BGMSourceVoice->Stop();
    }
}

void UAudioSystem::SetBGMVolume(float volume)
{
    if (BGMSourceVoice)
    {
        BGMSourceVoice->SetVolume(volume);
    }
}

void UAudioSystem::SetSFXVolume(float volume)
{
    if (SFXSubmixVoice)
    {
        SFXSubmixVoice->SetVolume(volume);
    }
}
