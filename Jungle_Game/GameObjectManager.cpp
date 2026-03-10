#pragma once
#include "GameObjectManager.h"

UGameObjectManager::~UGameObjectManager()
{
    for (UGameObject* obj : GameObjects)
    {
        delete obj;
    }
    GameObjects.clear();
    for (UGameObject* obj : PendingGameObjects)
    {
        delete obj;
    }
    PendingGameObjects.clear();
}

const std::vector<UGameObject*>& UGameObjectManager::GetGameObjects() const
{
    return GameObjects;
}

void UGameObjectManager::AddGameObject(UGameObject* obj)
{
    PendingGameObjects.push_back(obj);
}

void UGameObjectManager::Refresh()
{
    // delete inactive objects
    auto it = std::remove_if(GameObjects.begin(), GameObjects.end(),
        [](UGameObject* obj) { return obj->IsPendingDestroy(); });

    for (auto i = it; i != GameObjects.end(); ++i) {
        delete* i;
    }
    GameObjects.erase(it, GameObjects.end());

    // add pending objects
    if (!PendingGameObjects.empty()) {
        GameObjects.insert(GameObjects.end(), PendingGameObjects.begin(), PendingGameObjects.end());
        PendingGameObjects.clear();
    }
}
