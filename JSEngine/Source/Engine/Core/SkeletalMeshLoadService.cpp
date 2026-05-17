#include "Core/SkeletalMeshLoadService.h"

#include "Animation/AnimData/AnimSequence.h"
#include "Asset/Skeleton.h"
#include "Core/AssetPathPolicy.h"
#include "Core/Logging/Log.h"
#include "Core/Paths.h"
#include "Core/ResourceManager.h"
#include "Object/ObjectFactory.h"
#include "SimpleJSON/json.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
	FString SanitizeAssetFileName(const FString& Name)
	{
		FString Result = Name.empty() ? FString("AnimSequence") : Name;
		for (char& Ch : Result)
		{
			const bool bInvalid =
				Ch == '<' || Ch == '>' || Ch == ':' || Ch == '"' ||
				Ch == '/' || Ch == '\\' || Ch == '|' || Ch == '?' || Ch == '*';
			if (bInvalid || static_cast<unsigned char>(Ch) < 32)
			{
				Ch = '_';
			}
		}
		return Result;
	}

	FString MakeSequenceAssetPath(const FString& SourcePath, const UAnimSequence* Sequence, int32 SequenceIndex)
	{
		const std::filesystem::path SourceFsPath(FPaths::ToWide(FPaths::Normalize(SourcePath)));
		const FString MeshName = SanitizeAssetFileName(FPaths::ToUtf8(SourceFsPath.stem().wstring()));
		FString SequenceName = Sequence ? FString(Sequence->GetName()) : FString();
		if (SequenceName.empty())
		{
			SequenceName = "Sequence_" + std::to_string(SequenceIndex);
		}
		SequenceName = SanitizeAssetFileName(SequenceName);

		const std::filesystem::path AssetPath =
			std::filesystem::path(L"Asset") /
			L"Animation" /
			FPaths::ToWide(MeshName) /
			(FPaths::ToWide(SequenceName) + L".sequence");

		return FPaths::Normalize(FPaths::ToUtf8(AssetPath.generic_wstring()));
	}

	struct FSkeletalImportManifest
	{
		FString SourcePath;
		uint64 SourceFileWriteTime = 0;
		FString SkeletalMeshBinaryPath;
		FString SkeletonBinaryPath;
		FString PhysicsAssetBinaryPath;
		TArray<FString> AnimationSequencePaths;
	};

	uint64 ParseUInt64String(const FString& Value)
	{
		try
		{
			return static_cast<uint64>(std::stoull(Value));
		}
		catch (...)
		{
			return 0;
		}
	}

	bool LoadSkeletalImportManifest(const FString& ManifestPath, FSkeletalImportManifest& OutManifest)
	{
		std::ifstream ManifestFile(std::filesystem::path(FPaths::ToAbsolute(FPaths::ToWide(ManifestPath))));
		if (!ManifestFile.is_open())
		{
			return false;
		}

		FString FileContent((std::istreambuf_iterator<char>(ManifestFile)), std::istreambuf_iterator<char>());
		json::JSON Root = json::JSON::Load(FileContent);
		if (Root.JSONType() != json::JSON::Class::Object)
		{
			return false;
		}

		if (!Root.hasKey("Type") || Root["Type"].ToString() != "SkeletalImportManifest")
		{
			return false;
		}

		const long Version = Root.hasKey("Version") ? Root["Version"].ToInt() : 0;
		if (Version != 1)
		{
			return false;
		}

		OutManifest.SourcePath = Root.hasKey("SourcePath") ? FPaths::Normalize(Root["SourcePath"].ToString()) : FString();
		OutManifest.SourceFileWriteTime = Root.hasKey("SourceFileWriteTime")
			? ParseUInt64String(Root["SourceFileWriteTime"].ToString())
			: 0;
		OutManifest.SkeletalMeshBinaryPath = Root.hasKey("SkeletalMeshBinaryPath") ? FPaths::Normalize(Root["SkeletalMeshBinaryPath"].ToString()) : FString();
		OutManifest.SkeletonBinaryPath = Root.hasKey("SkeletonBinaryPath") ? FPaths::Normalize(Root["SkeletonBinaryPath"].ToString()) : FString();
		OutManifest.PhysicsAssetBinaryPath = Root.hasKey("PhysicsAssetBinaryPath") ? FPaths::Normalize(Root["PhysicsAssetBinaryPath"].ToString()) : FString();
		OutManifest.AnimationSequencePaths.clear();

		if (Root.hasKey("AnimationSequencePaths") &&
			Root["AnimationSequencePaths"].JSONType() == json::JSON::Class::Array)
		{
			json::JSON& SequencePaths = Root["AnimationSequencePaths"];
			for (int32 Index = 0; Index < SequencePaths.length(); ++Index)
			{
				OutManifest.AnimationSequencePaths.push_back(FPaths::Normalize(SequencePaths.at(static_cast<unsigned>(Index)).ToString()));
			}
		}

		return !OutManifest.SourcePath.empty() &&
			OutManifest.SourceFileWriteTime != 0 &&
			!OutManifest.SkeletalMeshBinaryPath.empty() &&
			!OutManifest.SkeletonBinaryPath.empty();
	}

	bool SaveSkeletalImportManifest(const FString& ManifestPath, const FSkeletalImportManifest& Manifest)
	{
		if (Manifest.SourcePath.empty() ||
			Manifest.SourceFileWriteTime == 0 ||
			Manifest.SkeletalMeshBinaryPath.empty() ||
			Manifest.SkeletonBinaryPath.empty())
		{
			return false;
		}

		json::JSON Root = json::JSON::Make(json::JSON::Class::Object);
		Root["Type"] = "SkeletalImportManifest";
		Root["Version"] = 1;
		Root["SourcePath"] = FPaths::Normalize(Manifest.SourcePath);
		Root["SourceFileWriteTime"] = std::to_string(Manifest.SourceFileWriteTime);
		Root["SkeletalMeshBinaryPath"] = FPaths::Normalize(Manifest.SkeletalMeshBinaryPath);
		Root["SkeletonBinaryPath"] = FPaths::Normalize(Manifest.SkeletonBinaryPath);
		Root["PhysicsAssetBinaryPath"] = FPaths::Normalize(Manifest.PhysicsAssetBinaryPath);

		Root["AnimationSequencePaths"] = json::Array();
		for (const FString& SequencePath : Manifest.AnimationSequencePaths)
		{
			Root["AnimationSequencePaths"].append(FPaths::Normalize(SequencePath));
		}

		std::error_code ErrorCode;
		const std::filesystem::path FilePath(FPaths::ToAbsolute(FPaths::ToWide(ManifestPath)));
		std::filesystem::create_directories(FilePath.parent_path(), ErrorCode);

		std::ofstream OutFile(FilePath);
		if (!OutFile.is_open())
		{
			return false;
		}

		OutFile << Root.dump(4);
		return OutFile.good();
	}

	bool IsSkeletalImportManifestFresh(
		const FSkeletalImportManifest& Manifest,
		const FString& NormalizedSourcePath,
		uint64 SourceWriteTime)
	{
		if (FPaths::Normalize(Manifest.SourcePath) != NormalizedSourcePath ||
			Manifest.SourceFileWriteTime != SourceWriteTime ||
			SourceWriteTime == 0)
		{
			return false;
		}

		if (!FAssetPathPolicy::FileExists(Manifest.SkeletalMeshBinaryPath) ||
			!FAssetPathPolicy::FileExists(Manifest.SkeletonBinaryPath))
		{
			return false;
		}

		if (!Manifest.PhysicsAssetBinaryPath.empty() &&
			!FAssetPathPolicy::FileExists(Manifest.PhysicsAssetBinaryPath))
		{
			return false;
		}

		for (const FString& SequencePath : Manifest.AnimationSequencePaths)
		{
			if (!FAssetPathPolicy::FileExists(SequencePath))
			{
				return false;
			}
		}

		return true;
	}
}

FSkeletalMeshLoadService::FSkeletalMeshLoadService(FResourceManager& InResourceManager)
	: ResourceManager(InResourceManager)
{
}

USkeletalMesh* FSkeletalMeshLoadService::Load(const FString& Path)
{
	const FString NormalizedPath = FPaths::Normalize(Path);

	if (USkeletalMesh* FoundMesh = ResourceManager.FindSkeletalMesh(NormalizedPath))
	{
		return FoundMesh;
	}

	ResourceManager.LoadMaterial(NormalizedPath, EMaterialShaderType::SurfaceLit);

	return LoadSourceOrCachedBinary(NormalizedPath);
}

USkeletalMesh* FSkeletalMeshLoadService::LoadSourceOrCachedBinary(const FString& NormalizedPath)
{
	FStaticMeshLoadOptions LoadOptions;
	const FString BinaryPath = FAssetPathPolicy::MakeWritableSkeletalMeshCacheBinaryPath(NormalizedPath);
	const FString SkeletonBinaryPath = FAssetPathPolicy::MakeWritableSkeletonCacheBinaryPath(NormalizedPath);
	const FString PhysicsAssetBinaryPath = FAssetPathPolicy::MakeWritablePhysicsAssetCacheBinaryPath(NormalizedPath);
	const FString ImportManifestPath = FAssetPathPolicy::MakeWritableSkeletalImportManifestPath(NormalizedPath);
	const uint64 SourceWriteTime = ResourceManager.GetFileWriteTimeTicks(NormalizedPath);

	FSkeletalMesh* LoadedMeshData = nullptr;
	TArray<UAnimSequence*> ImportedAnimationSequences;
	bool bLoadedFromImportManifest = false;
	bool bLoadedLegacyBinary = false;
	bool bSplitCacheReadyForManifest = false;
	double BinaryLoadSec = 0.0;
	double SourceLoadSec = 0.0;

	FSkeletalImportManifest CachedManifest;
	if (LoadSkeletalImportManifest(ImportManifestPath, CachedManifest) &&
		IsSkeletalImportManifestFresh(CachedManifest, NormalizedPath, SourceWriteTime))
	{
		const auto BinaryStart = std::chrono::steady_clock::now();

		LoadedMeshData = new FSkeletalMesh();
		if (!ResourceManager.BinarySerializer.LoadSkeletalMesh(CachedManifest.SkeletalMeshBinaryPath, *LoadedMeshData))
		{
			delete LoadedMeshData;
			LoadedMeshData = nullptr;
		}
		else if (!LoadedMeshData->HasValidSkeletonData())
		{
			USkeleton* LoadedSkeleton = UObjectManager::Get().CreateObject<USkeleton>();
			if (LoadedSkeleton && ResourceManager.BinarySerializer.LoadSkeleton(CachedManifest.SkeletonBinaryPath, *LoadedSkeleton))
			{
				LoadedMeshData->Skeleton = LoadedSkeleton;
				if (LoadedMeshData->SkeletonAssetPath.empty())
				{
					LoadedMeshData->SkeletonAssetPath = LoadedSkeleton->GetAssetPathFileName();
				}
			}
			else
			{
				delete LoadedMeshData;
				LoadedMeshData = nullptr;
			}
		}

		const auto BinaryEnd = std::chrono::steady_clock::now();
		BinaryLoadSec = std::chrono::duration<double>(BinaryEnd - BinaryStart).count();

		if (LoadedMeshData)
		{
			bLoadedFromImportManifest = true;
			UE_LOG("[SkeletalMeshLoad] Source=ImportManifest | Path=%s | BinarySec=%.6f | Manifest=%s",
			       NormalizedPath.c_str(), BinaryLoadSec, ImportManifestPath.c_str());
		}
	}

	// Manifest 도입 이전의 v1~v3 embedded .bin만 migration 후보로 수용한다.
	if (LoadedMeshData == nullptr && SourceWriteTime != 0)
	{
		FSkeletalMeshBinaryHeader BinaryHeader;
		const bool bHasBinaryHeader = ResourceManager.BinarySerializer.ReadSkeletalMeshHeader(BinaryPath, BinaryHeader);
		bLoadedLegacyBinary = bHasBinaryHeader &&
			BinaryHeader.Version < 4 &&
			BinaryHeader.SourceFileWriteTime == SourceWriteTime;
	}

	if (LoadedMeshData == nullptr && bLoadedLegacyBinary)
	{
		const auto BinaryStart = std::chrono::steady_clock::now();

		LoadedMeshData = new FSkeletalMesh();
		if (!ResourceManager.BinarySerializer.LoadSkeletalMesh(BinaryPath, *LoadedMeshData, &ImportedAnimationSequences))
		{
			delete LoadedMeshData;
			LoadedMeshData = nullptr;
		}

		if (LoadedMeshData && bLoadedLegacyBinary)
		{
			bool bSaveSkeletonOk = false;
			if (LoadedMeshData->Skeleton)
			{
				bSaveSkeletonOk = ResourceManager.BinarySerializer.SaveSkeleton(
					SkeletonBinaryPath,
					NormalizedPath,
					*LoadedMeshData->Skeleton);
			}

			if (bSaveSkeletonOk)
			{
				const bool bSaveConvertedMeshOk = ResourceManager.BinarySerializer.SaveSkeletalMesh(
					BinaryPath,
					NormalizedPath,
					*LoadedMeshData);
				bSplitCacheReadyForManifest = bSaveConvertedMeshOk;
				if (bSaveConvertedMeshOk)
				{
					UE_LOG("[SkeletalMeshLoad] Legacy skeletal cache converted | Path=%s | Skeleton=%s | Mesh=%s | Animations=%zu",
					       NormalizedPath.c_str(),
					       SkeletonBinaryPath.c_str(),
					       BinaryPath.c_str(),
					       ImportedAnimationSequences.size());
				}
				else
				{
					UE_LOG_WARNING("[SkeletalMeshLoad] Legacy skeletal mesh cache conversion failed; legacy cache kept | Path=%s | BinaryPath=%s",
					               NormalizedPath.c_str(),
					               BinaryPath.c_str());
				}
			}
			else
			{
				UE_LOG_WARNING("[SkeletalMeshLoad] Legacy skeleton cache conversion failed; legacy mesh cache kept | Path=%s | SkeletonBinaryPath=%s",
				               NormalizedPath.c_str(),
				               SkeletonBinaryPath.c_str());
			}
		}

		const auto BinaryEnd = std::chrono::steady_clock::now();
		BinaryLoadSec = std::chrono::duration<double>(BinaryEnd - BinaryStart).count();
	}

	// Binary/manifest 실패 또는 누락이면 FBX에서 import 후 split cache와 manifest를 굽는다.
	if (LoadedMeshData == nullptr)
	{
		const auto SourceStart = std::chrono::steady_clock::now();
		FImportedSkeletalAsset ImportedAsset = ResourceManager.FbxImporter.ImportSkeletalAsset(NormalizedPath, LoadOptions);
		LoadedMeshData = ImportedAsset.SkeletalMesh;
		ImportedAnimationSequences = ImportedAsset.AnimationSequences;
		const auto SourceEnd = std::chrono::steady_clock::now();
		SourceLoadSec = std::chrono::duration<double>(SourceEnd - SourceStart).count();

		if (!LoadedMeshData)
		{
			for (UAnimSequence* Sequence : ImportedAnimationSequences)
			{
				delete Sequence;
			}
			UE_LOG_ERROR("[SkeletalMeshLoad] Failed | Path=%s | BinarySec=%.6f | FbxSec=%.6f",
				NormalizedPath.c_str(), BinaryLoadSec, SourceLoadSec);
			return nullptr;
		}

		bool bSaveSkeletonOk = false;
		if (LoadedMeshData->Skeleton)
		{
			bSaveSkeletonOk = ResourceManager.BinarySerializer.SaveSkeleton(
				SkeletonBinaryPath,
				NormalizedPath,
				*LoadedMeshData->Skeleton);
			if (bSaveSkeletonOk)
			{
				UE_LOG("[SkeletalMeshLoad] Skeleton cache saved | Path=%s | BinaryPath=%s",
				       NormalizedPath.c_str(), SkeletonBinaryPath.c_str());
			}
			else
			{
				UE_LOG_WARNING("[SkeletalMeshLoad] Skeleton cache save failed | Path=%s | BinaryPath=%s",
				               NormalizedPath.c_str(), SkeletonBinaryPath.c_str());
			}
		}

		// Material 포인터는 직렬화 대상이 아니므로 이 시점에 그대로 굽고, resolve는 Finalize에서 한 번만.
		const bool bSaveBinaryOk = ResourceManager.BinarySerializer.SaveSkeletalMesh(BinaryPath, NormalizedPath, *LoadedMeshData);
		bSplitCacheReadyForManifest = bSaveSkeletonOk && bSaveBinaryOk;
		if (bSaveBinaryOk)
		{
			UE_LOG("[SkeletalMeshLoad] Source=FBX | Path=%s | FbxSec=%.6f | BinarySave=OK | BinaryPath=%s",
			       NormalizedPath.c_str(), SourceLoadSec, BinaryPath.c_str());
		}
		else
		{
			UE_LOG_WARNING("[SkeletalMeshLoad] Source=FBX | Path=%s | FbxSec=%.6f | BinarySave=FAIL | BinaryPath=%s",
			               NormalizedPath.c_str(), SourceLoadSec, BinaryPath.c_str());
		}
	}
	else
	{
		if (!bLoadedFromImportManifest)
		{
			UE_LOG("[SkeletalMeshLoad] Source=Binary | Path=%s | BinarySec=%.6f | BinaryPath=%s",
			       NormalizedPath.c_str(), BinaryLoadSec, BinaryPath.c_str());
		}
	}

	TArray<FString> SavedAnimationSequencePaths;
	USkeletalMesh* LoadedMesh = FinalizeLoadedMesh(
		LoadedMeshData,
		NormalizedPath,
		NormalizedPath,
		ImportedAnimationSequences,
		&SavedAnimationSequencePaths);

	const bool bAllImportedSequencesSaved = SavedAnimationSequencePaths.size() == ImportedAnimationSequences.size();
	if (LoadedMesh && bSplitCacheReadyForManifest && bAllImportedSequencesSaved)
	{
		FSkeletalImportManifest NewManifest;
		NewManifest.SourcePath = NormalizedPath;
		NewManifest.SourceFileWriteTime = SourceWriteTime;
		NewManifest.SkeletalMeshBinaryPath = BinaryPath;
		NewManifest.SkeletonBinaryPath = SkeletonBinaryPath;
		NewManifest.PhysicsAssetBinaryPath = FString();
		NewManifest.AnimationSequencePaths = SavedAnimationSequencePaths;

		if (SaveSkeletalImportManifest(ImportManifestPath, NewManifest))
		{
			UE_LOG("[SkeletalMeshLoad] Import manifest saved | Path=%s | Manifest=%s",
			       NormalizedPath.c_str(), ImportManifestPath.c_str());
		}
		else
		{
			UE_LOG_WARNING("[SkeletalMeshLoad] Failed to save import manifest | Path=%s | Manifest=%s",
			               NormalizedPath.c_str(), ImportManifestPath.c_str());
		}
	}
	else if (LoadedMesh && bSplitCacheReadyForManifest && !bAllImportedSequencesSaved)
	{
		UE_LOG_WARNING("[SkeletalMeshLoad] Import manifest skipped because not all animation sequences were saved | Path=%s | Saved=%zu | Imported=%zu",
		               NormalizedPath.c_str(),
		               SavedAnimationSequencePaths.size(),
		               ImportedAnimationSequences.size());
	}

	return LoadedMesh;
}

USkeletalMesh* FSkeletalMeshLoadService::FinalizeLoadedMesh(
	FSkeletalMesh* MeshData,
	const FString& ResolvePath,
	const FString& CacheKey,
	const TArray<UAnimSequence*>& ImportedAnimationSequences,
	TArray<FString>* OutSavedAnimationSequencePaths)
{
	ResourceManager.ResolveSkeletalMeshMaterialSlots(ResolvePath, MeshData);

	USkeletalMesh* LoadedMesh = UObjectManager::Get().CreateObject<USkeletalMesh>();
	LoadedMesh->SetMeshData(MeshData);

	ResourceManager.SkeletalMeshMap[CacheKey] = LoadedMesh;
	if (std::find(ResourceManager.SkeletalMeshFilePaths.begin(), ResourceManager.SkeletalMeshFilePaths.end(), CacheKey)
		== ResourceManager.SkeletalMeshFilePaths.end())
	{
		ResourceManager.SkeletalMeshFilePaths.push_back(CacheKey);
	}

	for (int32 SequenceIndex = 0; SequenceIndex < static_cast<int32>(ImportedAnimationSequences.size()); ++SequenceIndex)
	{
		UAnimSequence* Sequence = ImportedAnimationSequences[SequenceIndex];
		if (!Sequence)
		{
			continue;
		}

		if (!Sequence->GetSkeleton())
		{
			Sequence->SetSkeleton(LoadedMesh->GetSkeleton());
		}
		if (Sequence->GetSkeletonAssetPath().empty())
		{
			Sequence->SetSkeletonAssetPath(LoadedMesh->GetSkeletonAssetPath());
		}

		const FString SequenceAssetPath = MakeSequenceAssetPath(ResolvePath, Sequence, SequenceIndex);
		if (ResourceManager.SaveAnimSequence(SequenceAssetPath, Sequence))
		{
			if (OutSavedAnimationSequencePaths)
			{
				OutSavedAnimationSequencePaths->push_back(SequenceAssetPath);
			}
			UE_LOG("[SkeletalMeshLoad] Animation sequence asset saved | Mesh=%s | Sequence=%s | Path=%s",
			       CacheKey.c_str(),
			       Sequence->GetName().c_str(),
			       SequenceAssetPath.c_str());
		}
		else
		{
			UE_LOG_WARNING("[SkeletalMeshLoad] Failed to save animation sequence asset | Mesh=%s | Sequence=%s | Path=%s",
			               CacheKey.c_str(),
			               Sequence->GetName().c_str(),
			               SequenceAssetPath.c_str());
			delete Sequence;
		}
	}

	UE_LOG("[SkeletalMeshLoad] Loaded | Path=%s | Vertices=%zu | Indices=%zu | Bones=%zu | Sections=%zu",
	       CacheKey.c_str(),
	       LoadedMesh->GetVertices().size(),
	       LoadedMesh->GetIndices().size(),
	       LoadedMesh->GetBones().size(),
	       LoadedMesh->GetSections().size());

	return LoadedMesh;
}
