// 7z/UpdateSingle.h

#pragma once

#ifndef __7Z_UPDATEA_SINGLE_H
#define __7Z_UPDATEA_SINGLE_H

#include "7zOut.h"
#include "7zIn.h"
#include "7zUpdateItem.h"

#include "../../ICoder.h"
#include "../IArchive.h"

// #include "Common/Types.h"
// #include "Windows/PropVariant.h"

// #include "7zCompressionMode.h"

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
    bool useAdditionalHeaderStreams, 
    bool compressMainHeader,
    const CArchiveDatabaseEx &database,
    // const CRecordVector<bool> &compressStatuses,
    CObjectVector<CUpdateItem> &updateItems,
    // const CRecordVector<UINT32> &copyIndices,    
    IArchiveUpdateCallback *updateCallback);

}}

#endif
