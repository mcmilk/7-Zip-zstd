// GZip/Update.h

#pragma once

#ifndef __GZIP_UPDATE_H
#define __GZIP_UPDATE_H

#include "../IArchive.h"

#include "GZipOut.h"
#include "GZipItem.h"

namespace NArchive {
namespace NGZip {

struct CCompressionMethodMode
{
  UINT32 NumPasses;
  UINT32 NumFastBytes;
};

HRESULT UpdateArchive(IInStream *inStream, 
    UINT64 unpackSize,
    IOutStream *outStream,
    const CItem &newItem,
    const CCompressionMethodMode &compressionMethod,
    int indexInClient,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
