#include "StatePanel.h"

#include <map>

#include "imgui.h"
#include "Editor/EditorContext.h"
#include "Engine/EngineStatics.h"
#include "CoreUObject/Object.h"
#include "Engine/Asset/Asset.h"

namespace
{
	struct FTypeAllocationRow
	{
		uint32 Count = 0;
		size_t MemoryBytes = 0;
		uint32 AssetCount = 0;
		size_t AssetMemoryBytes = 0;
	};
}

const wchar_t* FStatePanel::GetPanelID() const
{
	return L"StatePanel";
}

const wchar_t* FStatePanel::GetDisplayName() const
{
	return L"State Panel";
}

void FStatePanel::Draw()
{
	if (ImGui::Begin("State Panel", nullptr))
	{
		ImGui::Text("FPS                     : %.1f  (%.3f ms)", GetContext()->CurrentFPS, GetContext()->DeltaTime * 1000.f);
		ImGui::Text("TotalAllocationCount    : %u", UEngineStatics::TotalAllocationCount);
		ImGui::Text("Heap Usage              : %.2f KB", UEngineStatics::TotalAllocatedBytes / 1024.f);

		std::map<FString, FTypeAllocationRow> TypeStats;
		uint32 TotalAssetCount = 0;
		size_t TotalAssetMemoryBytes = 0;

		for (UObject* Object : GUObjectArray)
		{
			if (Object == nullptr)
			{
				continue;
			}

			FTypeAllocationRow& TypeRow = TypeStats[Object->GetTypeName()];
			TypeRow.Count++;
			TypeRow.MemoryBytes += Object->GetStatMemoryBytes();

			if (Object->IsA(UAsset::GetClass()))
			{
				TypeRow.AssetCount++;
				TypeRow.AssetMemoryBytes += Object->GetStatMemoryBytes();
				TotalAssetCount++;
				TotalAssetMemoryBytes += Object->GetStatMemoryBytes();
			}
		}

		ImGui::Separator();
		ImGui::Text("Total UAsset Objects    : %u", TotalAssetCount);
		ImGui::Text("UAsset Memory Usage     : %.2f KB", static_cast<float>(TotalAssetMemoryBytes) / 1024.f);

		if (ImGui::CollapsingHeader("UObject Type Allocation", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::BeginTable("ObjectTypeAllocationTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
			{
				ImGui::TableSetupColumn("Type");
				ImGui::TableSetupColumn("Count");
				ImGui::TableSetupColumn("Memory(KB)");
				ImGui::TableSetupColumn("UAsset Count");
				ImGui::TableSetupColumn("UAsset Memory(KB)");
				ImGui::TableHeadersRow();

				for (const auto& [TypeName, Row] : TypeStats)
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%s", TypeName.c_str());
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%u", Row.Count);
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%.2f", static_cast<float>(Row.MemoryBytes) / 1024.f);
					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%u", Row.AssetCount);
					ImGui::TableSetColumnIndex(4);
					ImGui::Text("%.2f", static_cast<float>(Row.AssetMemoryBytes) / 1024.f);
				}

				ImGui::EndTable();
			}
		}

		if (ImGui::CollapsingHeader("Resource Load History", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const TArray<FResourceLoadHistoryEntry>& LoadHistory = UEngineStatics::GetResourceLoadHistory();
			ImGui::Text("Recorded Loads          : %zu", LoadHistory.size());

			if (ImGui::BeginTable("ResourceLoadHistoryTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0.f, 260.f)))
			{
				ImGui::TableSetupColumn("Type");
				ImGui::TableSetupColumn("Path");
				ImGui::TableSetupColumn("Load(ms)");
				ImGui::TableSetupColumn("Cache");
				ImGui::TableSetupColumn("Status");
				ImGui::TableHeadersRow();

				for (auto It = LoadHistory.rbegin(); It != LoadHistory.rend(); ++It)
				{
					const FResourceLoadHistoryEntry& Row = *It;
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%s", Row.AssetType.c_str());
					ImGui::TableSetColumnIndex(1);
					ImGui::TextWrapped("%s", Row.AssetPath.c_str());
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%.3f", Row.LoadMilliseconds);
					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%s", Row.bCacheHit ? "Hit" : "Cold");
					ImGui::TableSetColumnIndex(4);
					ImGui::Text("%s", Row.bSuccess ? "Success" : "Fail");
				}

				ImGui::EndTable();
			}
		}
	}
	ImGui::End();
}
