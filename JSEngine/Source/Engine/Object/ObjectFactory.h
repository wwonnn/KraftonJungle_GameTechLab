#pragma once

#include <functional>
#include <utility>
#include "Object/Object.h"
#include "Core/Singleton.h"

struct FObjectFactoryEntry
{
	std::function<UObject*()> Spawner;
	const UClass* Class = nullptr;
};

// Different from UFactory class
class FObjectFactory : public TSingleton<FObjectFactory>
{
	friend class TSingleton<FObjectFactory>;

public:
	void Register(const char* TypeName, std::function<UObject*()> Spawner, const UClass* Class = nullptr) {
		Registry[TypeName] = FObjectFactoryEntry{ std::move(Spawner), Class };
	}

	UObject* Create(const std::string& TypeName) {
		auto Spawner = Registry.find(TypeName);	// Do NOT use array accessor [] here. it will insert a new key if not found.
		return (Spawner != Registry.end() && Spawner->second.Spawner) ? Spawner->second.Spawner() : nullptr;
	}

	void GetRegisteredClasses(TArray<const UClass*>& OutClasses) const {
		for (const auto& [TypeName, Entry] : Registry)
		{
			if (Entry.Class)
			{
				OutClasses.push_back(Entry.Class);
			}
		}
	}

	void Shutdown() {
		Registry.clear();
		Registry.rehash(0);
	}

private:
	TMap<std::string, FObjectFactoryEntry> Registry;
};
