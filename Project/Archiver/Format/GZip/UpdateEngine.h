// GZip/UpdateEngine.h

#pragma once

#ifndef __GZIP_UPDATEAENGINE_H
#define __GZIP_UPDATEAENGINE_H

#include "Common/Vector.h"
#include "Common/Types.h"

#include "Archive/GZip/OutEngine.h"

#include "../Common/ArchiveInterface.h"
#include "CompressionMethod.h"
#include "Interface/ICoder.h"
#include "Archive/GZip/ItemInfoEx.h"

namespace NArchive {
namespace NGZip {

HRESULT UpdateArchive(IInStream *inStream, 
    // const NArchive::NGZip::CItemInfoEx *existingItemInfo,
    UINT64 unpackSize,
    IOutStream *outStream,
    const NArchive::NGZip::CItemInfo &newItemInfo,
    const CCompressionMethodMode &compressionMethod,
    int indexInClient,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
