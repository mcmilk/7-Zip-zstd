// BZip2/UpdateEngine.h

#pragma once

#ifndef __BZIP2_UPDATEAENGINE_H
#define __BZIP2_UPDATEAENGINE_H

#include "Common/Types.h"

#include "../Common/ArchiveInterface.h"

namespace NArchive {
namespace NBZip2 {

HRESULT UpdateArchive(
    UINT64 unpackSize,
    IOutStream *outStream,
    int indexInClient,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
