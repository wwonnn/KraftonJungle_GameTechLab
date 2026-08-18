#include "Scene.h"
#include "CollisionSystem.h"

void UScene::Refresh()
{
    for (auto it = GameObjects.begin(); it != GameObjects.end();)
    {
        UGameObject* obj = *it;

        if (!obj)
        {
            it = GameObjects.erase(it);
            continue;
        }

        if (obj->IsPendingDestroy())
        {
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
        if (obj)
        {
            GameObjects.push_back(obj);
        }
    }
    PendingGameObjects.clear();
}
