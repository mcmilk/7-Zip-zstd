// Zip/UpdateEngine.h

#pragma once

#ifndef __ZIP_UPDATEAENGINE_H
#define __ZIP_UPDATEAENGINE_H

#include "Common/Vector.h"
#include "Common/Types.h"

#include "Archive/Zip/OutEngine.h"

#include "UpdateItemInfo.h"
#include "../Common/ArchiveInterface.h"
#include "CompressionMethod.h"
#include "Interface/ICoder.h"
#include "Archive/Zip/ItemInfoEx.h"

namespace NArchive {
namespace NZip {

HRESULT CopyBlockToArchive(ISequentialInStream *inStream, 
    NArchive::NZip::COutArchive &outArchive, ICompressProgressInfo *progress);

HRESULT UpdateArchiveStd(NArchive::NZip::COutArchive &archive, 
    IInStream *inStream,
    const NArchive::NZip::CItemInfoExVector &inputItems,
    const CObjectVector<CUpdateItemInfo> &updateItems,
    const CCompressionMethodMode *options, 
    bool commentRangeAssigned,
    const CUpdateRange &commentRange,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
