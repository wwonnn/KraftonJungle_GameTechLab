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

        UpdateDelayedActions();
	}
}

float UTimer::GetTotalTime() {
	return totalTime;
}

float UTimer::GetDeltaTime() {
    return deltaTime;
}

void UTimer::ExecuteAfter(float seconds, std::function<void()> callback) {
    DelayedActions.push_back({ callback, seconds, totalTime, false });
}

void UTimer::UpdateDelayedActions() {
    for (auto it = DelayedActions.begin(); it != DelayedActions.end();) {
        if (!it->executed && totalTime - it->startTime >= it->delay) {
            it->callback();
            it->executed = true;
            it = DelayedActions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void UTimer::ClearDelayedActions() {
    DelayedActions.clear();
}
