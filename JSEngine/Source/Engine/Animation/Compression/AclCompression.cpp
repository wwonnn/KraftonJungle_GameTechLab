#include "Animation/Compression/AclCompression.h"

#include <acl/core/error_result.h>
#include <acl/compression/compress.h>
#include <acl/decompression/decompress.h>
#include <acl/version.h>
#include <rtm/qvvf.h>

#include <cstring>

static_assert(ACL_VERSION_MAJOR == 2, "Unexpected ACL major version");
static_assert(ACL_VERSION_MINOR == 1, "Unexpected ACL minor version");
static_assert(ACL_VERSION_PATCH == 0, "Unexpected ACL patch version");

FAclAnimSequenceRuntimeData::~FAclAnimSequenceRuntimeData()
{
    Reset();
}

acl::iallocator& FAclAnimSequenceRuntimeData::GetAllocator()
{
    return Allocator;
}

bool FAclAnimSequenceRuntimeData::TakeCompressedTracks(acl::compressed_tracks* InCompressedTracks)
{
    Reset();

    if (!InCompressedTracks)
    {
        return true;
    }

    const acl::error_result ValidationResult = InCompressedTracks->is_valid(false);
    if (ValidationResult.any())
    {
        UE_LOG_ERROR("[AclCompression] Invalid compressed tracks: %s", ValidationResult.c_str());
        Allocator.deallocate(InCompressedTracks, InCompressedTracks->get_size());
        return false;
    }

    CompressedTracks = InCompressedTracks;
    return RefreshInfo();
}

bool FAclAnimSequenceRuntimeData::CopyCompressedTracks(const void* Data, uint32 SizeBytes)
{
    Reset();

    if (!Data || SizeBytes == 0)
    {
        return false;
    }

    void* Buffer = Allocator.allocate(SizeBytes, alignof(acl::compressed_tracks));
    if (!Buffer)
    {
        UE_LOG_ERROR("[AclCompression] Failed to allocate compressed track buffer | Bytes=%u", SizeBytes);
        return false;
    }

    std::memcpy(Buffer, Data, SizeBytes);

    acl::error_result MakeResult;
    acl::compressed_tracks* Tracks = acl::make_compressed_tracks(Buffer, &MakeResult);
    if (!Tracks)
    {
        UE_LOG_ERROR("[AclCompression] Invalid compressed track buffer: %s", MakeResult.c_str());
        Allocator.deallocate(Buffer, SizeBytes);
        return false;
    }

    CompressedTracks = Tracks;
    return RefreshInfo();
}

void FAclAnimSequenceRuntimeData::Reset()
{
    if (CompressedTracks)
    {
        Allocator.deallocate(CompressedTracks, CompressedTracks->get_size());
        CompressedTracks = nullptr;
    }

    Info = FAclCompressedClipInfo{};
}

bool FAclAnimSequenceRuntimeData::IsValid() const
{
    return CompressedTracks != nullptr;
}

const FAclCompressedClipInfo& FAclAnimSequenceRuntimeData::GetInfo() const
{
    return Info;
}

const acl::compressed_tracks* FAclAnimSequenceRuntimeData::GetCompressedTracks() const
{
    return CompressedTracks;
}

acl::compressed_tracks* FAclAnimSequenceRuntimeData::GetMutableCompressedTracks()
{
    return CompressedTracks;
}

const uint8* FAclAnimSequenceRuntimeData::GetCompressedData() const
{
    return reinterpret_cast<const uint8*>(CompressedTracks);
}

uint32 FAclAnimSequenceRuntimeData::GetCompressedSizeBytes() const
{
    return Info.SizeBytes;
}

bool FAclAnimSequenceRuntimeData::RefreshInfo()
{
    if (!CompressedTracks)
    {
        Info = FAclCompressedClipInfo{};
        return false;
    }

    const acl::error_result ValidationResult = CompressedTracks->is_valid(false);
    if (ValidationResult.any())
    {
        UE_LOG_ERROR("[AclCompression] Invalid compressed tracks metadata: %s", ValidationResult.c_str());
        Reset();
        return false;
    }

    Info.SizeBytes = CompressedTracks->get_size();
    Info.Hash = CompressedTracks->get_hash();
    Info.TrackCount = CompressedTracks->get_num_tracks();
    Info.SamplesPerTrack = CompressedTracks->get_num_samples_per_track();
    Info.SampleRate = CompressedTracks->get_sample_rate();
    Info.Duration = CompressedTracks->get_duration();
    return true;
}

FAclLibraryInfo FAclCompression::GetLibraryInfo()
{
    (void)sizeof(acl::compression_settings);
    (void)sizeof(acl::decompression_context<acl::default_transform_decompression_settings>);
    (void)sizeof(rtm::qvvf);

    return FAclLibraryInfo{
        static_cast<int32>(ACL_VERSION_MAJOR),
        static_cast<int32>(ACL_VERSION_MINOR),
        static_cast<int32>(ACL_VERSION_PATCH)
    };
}
