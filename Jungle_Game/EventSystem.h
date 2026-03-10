#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class EventSystem
{
public:
    static EventSystem& Get()
    {
        static EventSystem instance;
        return instance;
    }

    EventSystem() = default;
    ~EventSystem() = default;

public:
    void Subscribe(const std::string& eventName, std::function<void()> callback)
    {
        Listeners[eventName] = callback;
    }

    void UnSubscribe(const std::string& eventName)
    {
        Listeners.erase(eventName);
    }

    void Trigger(const std::string& eventName)
    {
        if (Listeners.find(eventName) != Listeners.end())
        {
            Listeners[eventName]();
        }
    }

private:
    std::unordered_map<std::string, std::function<void()>> Listeners;
};

