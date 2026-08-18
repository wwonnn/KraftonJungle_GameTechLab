#pragma once
#include <windows.h>
#include <functional>
#include <vector>

struct FDelayedAction
{
    std::function<void()> callback;
    float delay;
    float startTime;
    bool executed = false;
};

class UTimer 
{
private:
    LARGE_INTEGER frequency;
    LARGE_INTEGER prevTime;
    LARGE_INTEGER currTime;

    float deltaTime;
    float totalTime;

    bool bTimerStart;

    std::vector<FDelayedAction> DelayedActions;

public:
    UTimer();
    ~UTimer();
    void Setup();
    void Update();
    float GetTotalTime();
    float GetDeltaTime();

    void ExecuteAfter(float seconds, std::function<void()> callback);
    void UpdateDelayedActions();
    void ClearDelayedActions();
};
