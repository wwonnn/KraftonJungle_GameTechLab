#pragma once
#include <unordered_map>
#include <string>

#include "GameObject.h"

struct FItemTemplate {
    int ID;
    std::string Name;
    std::string TextureName;
    FVector Veclocity;
    FVector Scale;
    float DestroyDelay;
};

class UItemDatabase {
private:
    std::unordered_map<int, FItemTemplate> Data;

public:
    static UItemDatabase& Get()
    {
        static UItemDatabase instance;
        return instance;
    }

    UItemDatabase() { Init(); }
    ~UItemDatabase() = default;

    void Init() {
        Data[0] = { 0, std::string("heart"), std::string("heart"), FVector(0.0f, -0.4f, 0.0f), FVector(0.05f, 0.05f, 1.0f), 3.5f };
        Data[1] = { 1, "upgrade", "upgrade", FVector(0.0f, -0.6f, 0.0f), FVector(0.04f, 0.04f, 1.0f), 3.0f};
    }

    const FItemTemplate& GetItem(int id) { return Data.at(id); }
};
