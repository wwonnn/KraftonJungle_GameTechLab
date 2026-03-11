#pragma once
#include <unordered_map>
#include <string>

#include "GameObject.h"

enum class EItemType
{
    Heal,
    Upgrade,
};

struct FItemTemplate {
    EItemType ItemType;
    std::string Name;
    std::string TextureName;
    FVector Veclocity;
    FVector Scale;
    float DestroyDelay;
};

class UItemDatabase {
private:
    std::unordered_map<EItemType, FItemTemplate> Data;

public:
    static UItemDatabase& Get()
    {
        static UItemDatabase instance;
        return instance;
    }

    UItemDatabase() { Init(); }
    ~UItemDatabase() = default;

    void Init() {
        Data[EItemType::Heal] = { EItemType::Heal, std::string("heart"), std::string("heart"), FVector(0.0f, -0.4f, 0.0f), FVector(0.05f, 0.05f, 1.0f), 3.5f };
        Data[EItemType::Upgrade] = { EItemType::Upgrade, "upgrade", "upgrade", FVector(0.0f, -0.6f, 0.0f), FVector(0.04f, 0.04f, 1.0f), 3.0f};
    }

    const FItemTemplate& GetItem(EItemType itemtype) { return Data.at(itemtype); }
};
