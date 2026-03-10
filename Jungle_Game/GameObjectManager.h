#pragma once
#include "GameObject.h"
#include <vector>

class UGameObjectManager {
public:
    static UGameObjectManager& Get() {
        static UGameObjectManager instance;
        return instance;
    }

    UGameObjectManager() = default;
    ~UGameObjectManager();

public:
    const std::vector <UGameObject*>& GetGameObjects() const;
    void AddGameObject(UGameObject* obj);
    void Refresh();

private:
    std::vector<UGameObject*> GameObjects;
    std::vector<UGameObject*> PendingGameObjects;
};
