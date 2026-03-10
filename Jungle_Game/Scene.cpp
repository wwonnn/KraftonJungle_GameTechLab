#include "Scene.h"
#include "CollisionSystem.h"

void UScene::Refresh()
{
    for (auto it = GameObjects.begin(); it != GameObjects.end();)
    {
        UGameObject* obj = *it;
        if (obj->IsPendingDestroy())
        {
            UCollisionSystem::Get().ClearCollisionPair(obj);
            delete obj;
            it = GameObjects.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (UGameObject* obj : PendingGameObjects)
    {
        GameObjects.push_back(obj);
    }
    PendingGameObjects.clear();
}
