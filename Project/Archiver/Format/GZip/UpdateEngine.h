// GZip/UpdateEngine.h

#pragma once

#ifndef __GZIP_UPDATEAENGINE_H
#define __GZIP_UPDATEAENGINE_H

#include "Common/Vector.h"
#include "Common/Types.h"

#include "Archive/GZip/OutEngine.h"

#include "../../Common/IArchiveHandler2.h"
#include "CompressionMethod.h"
#include "Interface/ICoder.h"
#include "Archive/GZip/ItemInfoEx.h"

namespace NArchive {
namespace NGZip {

HRESULT UpdateArchive(IInStream *anInStream, 
    const NArchive::NGZip::CItemInfoEx *anItemInfoExist,
    UINT64 anUnpackSize,
    IOutStream *anOutStream,
    const NArchive::NGZip::CItemInfo &aNewItemInfo,
    const CCompressionMethodMode &aCompressionMethod,
    int anIndexInClient,
    IUpdateCallBack *anUpdateCallBack);

}}

#endif
