#pragma once

#include "fmod/fmod.hpp"
#include <unordered_map>
#include <string>

#pragma comment(lib, "fmodL_vc.lib")

class AudioSystem
{
public:
	AudioSystem() = default;
	~AudioSystem() = default;

public:
	bool Create();
	void Release();

	void LoadFromFile(const std::string& filePath, const std::string& soundName);
	void Play(const std::string& soundName);

private:
	FMOD::System* System = nullptr;

	std::unordered_map<std::string, FMOD::Sound*> SoundMap;
};

