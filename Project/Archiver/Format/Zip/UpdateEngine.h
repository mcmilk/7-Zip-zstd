// Zip/UpdateEngine.h

#pragma once

#ifndef __ZIP_UPDATEAENGINE_H
#define __ZIP_UPDATEAENGINE_H

#include "Common/Vector.h"
#include "Common/Types.h"

#include "Archive/Zip/OutEngine.h"

#include "UpdateItemInfo.h"
#include "../Common/IArchiveHandler.h"
#include "CompressionMethod.h"
#include "Interface/ICoder.h"
#include "Archive/Zip/ItemInfoEx.h"

namespace NArchive {
namespace NZip {

HRESULT CopyBlockToArchive(ISequentialInStream *anInStream, 
    NArchive::NZip::COutArchive &anOutArchive, ICompressProgressInfo *aProgress);

HRESULT UpdateArchiveStd(NArchive::NZip::COutArchive &anArchive, 
    IInStream *anInStream,
    const NArchive::NZip::CItemInfoExVector &anInputItems,
    const CRecordVector<bool> &aCompressStatuses,
    const CObjectVector<CUpdateItemInfo> &anUpdateItems,
    const CRecordVector<UINT32> &aCopyIndexes,
    const CCompressionMethodMode *anOptions, 
    bool aCommentRangeAssigned,
    const CUpdateRange &aCommentRange,
    IUpdateCallBack *anUpdateCallBack);

}}

#endif
