// UpdateMain.h

#pragma once

#ifndef __7Z_UPDATEMAIN_H
#define __7Z_UPDATEMAIN_H

#include "Interface/IInOutStreams.h"
#include "InEngine.h"
#include "UpdateEngine.h"
#include "UpdateSolidEngine.h"
#include "CompressionMethod.h"

namespace NArchive {
namespace N7z {

HRESULT UpdateMain(const NArchive::N7z::CArchiveDatabaseEx &aDatabase,
    const CRecordVector<bool> &aCompressStatuses,
    const CObjectVector<CUpdateItemInfo> &anUpdateItems,
    const CRecordVector<UINT32> &aCopyIndexes,
    IOutStream *anOutStream,
    IInStream *anInStream,
    NArchive::N7z::CInArchiveInfo *anInArchiveInfo,
    CCompressionMethodMode *aMethod,
    CCompressionMethodMode *aHeaderMethod,
    IUpdateCallBack *anUpdateCallBack,
    bool aSolid);

}}

#endif
