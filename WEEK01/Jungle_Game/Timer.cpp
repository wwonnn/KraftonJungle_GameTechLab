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
    std::vector<std::function<void()>> readyCallbacks;

    for (size_t i = 0; i < DelayedActions.size(); ++i)
    {
        if (!DelayedActions[i].executed && totalTime - DelayedActions[i].startTime >= DelayedActions[i].delay)
        {
            readyCallbacks.push_back(DelayedActions[i].callback);
            DelayedActions[i].executed = true;
        }
    }

    DelayedActions.erase(
        std::remove_if(DelayedActions.begin(), DelayedActions.end(),
            [](const FDelayedAction& action) { return action.executed; }),
        DelayedActions.end()
    );

    for (auto& callback : readyCallbacks)
    {
        callback();
    }
}

void UTimer::ClearDelayedActions() {
    DelayedActions.clear();
}
