// GZip/Update.h

#ifndef __GZIP_UPDATE_H
#define __GZIP_UPDATE_H

#include "../IArchive.h"

#include "GZipOut.h"
#include "GZipItem.h"

namespace NArchive {
namespace NGZip {

struct CCompressionMethodMode
{
  UInt32 NumPasses;
  UInt32 NumFastBytes;
};

HRESULT UpdateArchive(IInStream *inStream, 
    UInt64 unpackSize,
    ISequentialOutStream *outStream,
    const CItem &newItem,
    const CCompressionMethodMode &compressionMethod,
    int indexInClient,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
