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

HRESULT CopyBlock(ISequentialInStream *inStream, 
    ISequentialOutStream *outStream, ICompressProgressInfo *progress);

HRESULT UpdateArchiveStd(NArchive::N7z::COutArchive &archive, 
    IInStream *inStream,
    const CCompressionMethodMode *options, 
    const CCompressionMethodMode *headerMethod, 
    const NArchive::N7z::CArchiveDatabaseEx &database,
    const CRecordVector<bool> &compressStatuses,
    const CObjectVector<CUpdateItemInfo> &updateItems,
    const CRecordVector<UINT32> &copyIndices,    
    IUpdateCallBack *updateCallback);

}}

#endif
