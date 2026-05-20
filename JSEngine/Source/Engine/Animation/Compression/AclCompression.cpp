#include "Animation/Compression/AclCompression.h"

#include <acl/compression/compress.h>
#include <acl/decompression/decompress.h>
#include <acl/version.h>
#include <rtm/qvvf.h>

static_assert(ACL_VERSION_MAJOR == 2, "Unexpected ACL major version");
static_assert(ACL_VERSION_MINOR == 1, "Unexpected ACL minor version");
static_assert(ACL_VERSION_PATCH == 0, "Unexpected ACL patch version");

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
