#include "InputManager.h"

void UInputManager::Update()
{
	for (int i = 0; i < 256; ++i)
	{
		PreviousKeys[i] = Keys[i];
		Keys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
	}
}

bool UInputManager::GetKey(int key)
{
	return Keys[key];
}

bool UInputManager::GetKeyDown(int key)
{
	return Keys[key] && !PreviousKeys[key];
}

bool UInputManager::GetKeyUp(int key)
{
	return !Keys[key] && PreviousKeys[key];
}