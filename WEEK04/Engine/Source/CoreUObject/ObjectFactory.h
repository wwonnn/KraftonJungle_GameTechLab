#pragma once

#include "Core/CoreMinimal.h"
#include <functional>

class UObject;

struct FObjectFactory
{
public:
    ENGINE_API static UObject* ConstructObject(const void* Id);
    ENGINE_API static void RegisterObjectType(const void* Id, std::function<UObject*()> FactoryFunction);

private:
    static TMap<const void*, std::function<UObject*()>>& GetRegistry();
};