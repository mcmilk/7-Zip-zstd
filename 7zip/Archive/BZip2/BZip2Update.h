// BZip2Update.h

#pragma once

#ifndef __BZIP2_UPDATE_H
#define __BZIP2_UPDATE_H

#include "Common/Types.h"

#include "../IArchive.h"

namespace NArchive {
namespace NBZip2 {

HRESULT UpdateArchive(
    UINT64 unpackSize,
    IOutStream *outStream,
    int indexInClient,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
