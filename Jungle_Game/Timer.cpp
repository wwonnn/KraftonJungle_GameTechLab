#include "Timer.h"

Timer::Timer() {
	QueryPerformanceFrequency(&frequency);
	deltaTime = 0.0f;
	totalTime = 0.0f;
	bTimerStart = false;
}

void Timer::Setup() {
	QueryPerformanceCounter(&prevTime);
	bTimerStart = true;
}

void Timer::Update() {
	if (bTimerStart) {
		QueryPerformanceCounter(&currTime);

		deltaTime = (double)(currTime.QuadPart - prevTime.QuadPart) / frequency.QuadPart;

		prevTime = currTime;
		totalTime += deltaTime;
	}
}

float Timer::GetTotalTime() {
	return totalTime;
}

float Timer::GetDeltaTime() {
    return deltaTime;
}
