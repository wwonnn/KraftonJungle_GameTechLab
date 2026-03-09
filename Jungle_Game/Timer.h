#include <windows.h>

class Timer 
{
private:
    LARGE_INTEGER frequency;
    LARGE_INTEGER prevTime;
    LARGE_INTEGER currTime;

    float deltaTime;
    float totalTime;

    bool bTimerStart;

public:
    Timer();
    void Setup();
    void Update();
    float GetTotalTime();
    float GetDeltaTime();
};
