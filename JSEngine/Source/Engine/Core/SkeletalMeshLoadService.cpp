#include "Core/SkeletalMeshLoadService.h"

#include "Animation/AnimData/AnimSequence.h"
#include "Asset/Skeleton.h"
#include "Core/AssetPathPolicy.h"
#include "Core/Logging/Log.h"
#include "Core/Paths.h"
#include "Core/ResourceManager.h"
#include "Object/ObjectFactory.h"

#include <algorithm>
#include <chrono>
#include <filesystem>

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

	FSkeletalMesh* LoadedMeshData = nullptr;
	TArray<UAnimSequence*> ImportedAnimationSequences;
	bool bLoadedLegacyBinary = false;
	double BinaryLoadSec = 0.0;
	double SourceLoadSec = 0.0;

	// 1) Binary 캐시가 소스보다 신선하면 그걸 우선 시도.
	if (ResourceManager.IsSkeletalMeshBinaryValid(NormalizedPath, BinaryPath))
	{
		const auto BinaryStart = std::chrono::steady_clock::now();

		FSkeletalMeshBinaryHeader BinaryHeader;
		const bool bHasBinaryHeader = ResourceManager.BinarySerializer.ReadSkeletalMeshHeader(BinaryPath, BinaryHeader);
		bLoadedLegacyBinary = bHasBinaryHeader && BinaryHeader.Version < 4;

		LoadedMeshData = new FSkeletalMesh();
		if (!ResourceManager.BinarySerializer.LoadSkeletalMesh(
				BinaryPath,
				*LoadedMeshData,
				bLoadedLegacyBinary ? &ImportedAnimationSequences : nullptr))
		{
			delete LoadedMeshData;
			LoadedMeshData = nullptr;
		}
		else if (!LoadedMeshData->HasValidSkeletonData())
		{
			USkeleton* LoadedSkeleton = UObjectManager::Get().CreateObject<USkeleton>();
			if (LoadedSkeleton && ResourceManager.BinarySerializer.LoadSkeleton(SkeletonBinaryPath, *LoadedSkeleton))
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

	// 2) Binary 실패/누락이면 FBX에서 import 후 캐시 굽기.
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

		if (LoadedMeshData->Skeleton)
		{
			const bool bSaveSkeletonOk = ResourceManager.BinarySerializer.SaveSkeleton(
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
		UE_LOG("[SkeletalMeshLoad] Source=Binary | Path=%s | BinarySec=%.6f | BinaryPath=%s",
		       NormalizedPath.c_str(), BinaryLoadSec, BinaryPath.c_str());
	}

	return FinalizeLoadedMesh(LoadedMeshData, NormalizedPath, NormalizedPath, ImportedAnimationSequences);
}

USkeletalMesh* FSkeletalMeshLoadService::FinalizeLoadedMesh(
	FSkeletalMesh* MeshData,
	const FString& ResolvePath,
	const FString& CacheKey,
	const TArray<UAnimSequence*>& ImportedAnimationSequences)
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
