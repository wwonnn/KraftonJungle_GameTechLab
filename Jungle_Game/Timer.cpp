#include "Timer.h"

UTimer::UTimer() {
	QueryPerformanceFrequency(&frequency);
	deltaTime = 0.0f;
	totalTime = 0.0f;
	bTimerStart = false;
}

UTimer::~UTimer() {
}

void UTimer::Setup() {
	QueryPerformanceCounter(&prevTime);
	bTimerStart = true;
}

void UTimer::Update() {
	if (bTimerStart) {
		QueryPerformanceCounter(&currTime);

		deltaTime = (double)(currTime.QuadPart - prevTime.QuadPart) / frequency.QuadPart;
        if (deltaTime > 0.2) deltaTime = 0.2;

		prevTime = currTime;
		totalTime += deltaTime;
	}
}

float UTimer::GetTotalTime() {
	return totalTime;
}

float UTimer::GetDeltaTime() {
    return deltaTime;
}
