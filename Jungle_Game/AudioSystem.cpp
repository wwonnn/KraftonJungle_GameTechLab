#include "AudioSystem.h"

bool UAudioSystem::Create()
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

void UAudioSystem::Release()
{
	if (System)
	{
		System->release();
		System = nullptr;
	}
}

void UAudioSystem::LoadFromFile(const std::string& filePath, const std::string& soundName)
{
	FMOD::Sound* sound;
	FMOD_RESULT result = System->createSound(filePath.c_str(), FMOD_DEFAULT, nullptr, &sound);
	if (result != FMOD_OK)
	{
		return;
	}
	SoundMap[soundName] = sound;
}

void UAudioSystem::Play(const std::string& soundName)
{
	auto iter = SoundMap.find(soundName);
	if (iter != SoundMap.end())
	{
		System->playSound(iter->second, nullptr, false, nullptr);
	}
}
