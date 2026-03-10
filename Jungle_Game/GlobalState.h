#pragma once

class GlobalState
{
public:
    static GlobalState& Get()
    {
        static GlobalState instance;
        return instance;
    }
    GlobalState() = default;
    ~GlobalState() = default;

public:
    bool bAllStageCleared = false;
};

