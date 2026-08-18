#pragma once

#include <windows.h>

class UInputManager
{
public:
	static UInputManager& Get()
	{
		static UInputManager instance;
		return instance;
	}

	UInputManager() = default;
	~UInputManager() = default;

public:
	void Update();

	bool GetKey(int key);
	bool GetKeyDown(int key);
	bool GetKeyUp(int key);

private:
	bool Keys[256] = { false };
	bool PreviousKeys[256] = { false };
};
