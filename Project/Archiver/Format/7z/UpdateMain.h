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

HRESULT UpdateMain(const NArchive::N7z::CArchiveDatabaseEx &database,
    const CRecordVector<bool> &compressStatuses,
    const CObjectVector<CUpdateItemInfo> &updateItems,
    const CRecordVector<UINT32> &copyIndices,
    IOutStream *outStream,
    IInStream *inStream,
    NArchive::N7z::CInArchiveInfo *inArchiveInfo,
    CCompressionMethodMode *method,
    CCompressionMethodMode *headerMethod,
    IUpdateCallBack *anUpdateCallback,
    bool solid);

}}

#endif
