// UpdateMain.h

#pragma once

#ifndef __ZIP_UPDATEMAIN_H
#define __ZIP_UPDATEMAIN_H

#include "Interface/IInOutStreams.h"
#include "Archive/Zip/InEngine.h"
#include "UpdateEngine.h"

namespace NArchive {
namespace NZip {

HRESULT UpdateMain(
    const NArchive::NZip::CItemInfoExVector &anInputItems,
    const CRecordVector<bool> &aCompressStatuses,
    const CObjectVector<CUpdateItemInfo> &anUpdateItems,
    const CRecordVector<UINT32> &aCopyIndexes,
    IOutStream *anOutStream,
    NArchive::NZip::CInArchive *anInArchive,
    CCompressionMethodMode *aCompressionMethodMode,
    IUpdateCallBack *anUpdateCallBack);

}}

#endif
