// BZip2Update.h

#ifndef __BZIP2_UPDATE_H
#define __BZIP2_UPDATE_H

#include "../IArchive.h"
#include "../../Common/CreateCoder.h"

namespace NArchive {
namespace NBZip2 {

HRESULT UpdateArchive(
    DECL_EXTERNAL_CODECS_LOC_VARS
    UInt64 unpackSize,
    ISequentialOutStream *outStream,
    int indexInClient,
    UInt32 dictionary,
    UInt32 numPasses,
    #ifdef COMPRESS_MT
    UInt32 numThreads,
    #endif
    IArchiveUpdateCallback *updateCallback);

}}

#endif
