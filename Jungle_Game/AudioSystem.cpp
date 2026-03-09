#include "AudioSystem.h"

bool AudioSystem::Create()
{
	FMOD_RESULT result;
	result = FMOD::System_Create(&System);
	if (result != FMOD_OK)
	{
		return false;
	}

	result = System->init(512, FMOD_INIT_NORMAL, nullptr);
	if (result != FMOD_OK)
	{
		return false;
	}

	return true;
}

void AudioSystem::Release()
{
	if (System)
	{
		System->release();
		System = nullptr;
	}
}

void AudioSystem::LoadFromFile(const std::string& filePath, const std::string& soundName)
{
	FMOD::Sound* sound;
	FMOD_RESULT result = System->createSound(filePath.c_str(), FMOD_DEFAULT, nullptr, &sound);
	if (result != FMOD_OK)
	{
		return;
	}
	SoundMap[soundName] = sound;
}

void AudioSystem::Play(const std::string& soundName)
{
	auto iter = SoundMap.find(soundName);
	if (iter != SoundMap.end())
	{
		System->playSound(iter->second, nullptr, false, nullptr);
	}
}
