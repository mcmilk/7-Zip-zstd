// 7z/UpdateEngine.h

#pragma once

#ifndef __7Z_UPDATEAENGINE_H
#define __7Z_UPDATEAENGINE_H

#include "Common/Vector.h"
#include "Common/Types.h"

#include "ItemInfo.h"
#include "MethodInfo.h"
#include "InEngine.h"
#include "OutEngine.h"

#include "../Common/IArchiveHandler.h"
#include "CompressionMethod.h"
#include "Interface/ICoder.h"
#include "Windows/PropVariant.h"
#include "UpdateItemInfo.h"

namespace NArchive {
namespace N7z {

struct CEncodingInfo: public NArchive::N7z::CCoderInfo
{
};

HRESULT CopyBlock(ISequentialInStream *anInStream, 
    ISequentialOutStream *anOutStream, ICompressProgressInfo *aProgress);

HRESULT UpdateArchiveStd(NArchive::N7z::COutArchive &anArchive, 
    IInStream *anInStream,
    const CCompressionMethodMode *anOptions, 
    const CCompressionMethodMode *aHeaderMethod, 
    const NArchive::N7z::CArchiveDatabaseEx &aDatabase,
    const CRecordVector<bool> &aCompressStatuses,
    const CObjectVector<CUpdateItemInfo> &anUpdateItems,
    const CRecordVector<UINT32> &aCopyIndexes,    
    IUpdateCallBack *anUpdateCallBack);

}}

#endif
