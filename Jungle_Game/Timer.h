#pragma once
#include <windows.h>

class UTimer 
{
private:
    LARGE_INTEGER frequency;
    LARGE_INTEGER prevTime;
    LARGE_INTEGER currTime;

    float deltaTime;
    float totalTime;

    bool bTimerStart;

public:
    UTimer();
    ~UTimer();
    void Setup();
    void Update();
    float GetTotalTime();
    float GetDeltaTime();
};
