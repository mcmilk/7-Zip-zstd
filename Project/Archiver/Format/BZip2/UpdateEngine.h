// BZip2/UpdateEngine.h

#pragma once

#ifndef __BZIP2_UPDATEAENGINE_H
#define __BZIP2_UPDATEAENGINE_H

#include "Common/Types.h"

#include "../../Common/IArchiveHandler2.h"

namespace NArchive {
namespace NBZip2 {

HRESULT UpdateArchive(
    UINT64 anUnpackSize,
    IOutStream *anOutStream,
    int anIndexInClient,
    IUpdateCallBack *anUpdateCallBack);

}}

#endif
