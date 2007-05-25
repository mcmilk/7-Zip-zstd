// GZip/Update.h

#ifndef __GZIP_UPDATE_H
#define __GZIP_UPDATE_H

#include "../IArchive.h"

#include "../../Common/CreateCoder.h"

#include "GZipOut.h"
#include "GZipItem.h"

namespace NArchive {
namespace NGZip {

struct CCompressionMethodMode
{
  UInt32 NumPasses;
  UInt32 NumFastBytes;
  UInt32 Algo;
  bool NumMatchFinderCyclesDefined;
  UInt32 NumMatchFinderCycles;
};

HRESULT UpdateArchive(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IInStream *inStream, 
    UInt64 unpackSize,
    ISequentialOutStream *outStream,
    const CItem &newItem,
    const CCompressionMethodMode &compressionMethod,
    int indexInClient,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
