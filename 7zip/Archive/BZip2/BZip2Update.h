// BZip2Update.h

#ifndef __BZIP2_UPDATE_H
#define __BZIP2_UPDATE_H

#include "../IArchive.h"

namespace NArchive {
namespace NBZip2 {

HRESULT UpdateArchive(
    UInt64 unpackSize,
    ISequentialOutStream *outStream,
    int indexInClient,
    UInt32 numPasses,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
