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
    CObjectVector<CUpdateItemInfo> &updateItems,
    IOutStream *outStream,
    IInStream *inStream,
    NArchive::N7z::CInArchiveInfo *inArchiveInfo,
    CCompressionMethodMode *method,
    CCompressionMethodMode *headerMethod,
    bool useAdditionalHeaderStreams, 
    bool compressMainHeader,
    IArchiveUpdateCallback *anUpdateCallback,
    bool solid,
    bool removeSfxBlock);

}}

#endif
