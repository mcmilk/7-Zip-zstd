// 7zUpdate.h

#pragma once

#ifndef __7Z_UPDATE_H
#define __7Z_UPDATE_H

#include "7zIn.h"
#include "7zCompressionMode.h"
#include "7zUpdateItem.h"

#include "../IArchive.h"

namespace NArchive {
namespace N7z {

HRESULT UpdateMain(const NArchive::N7z::CArchiveDatabaseEx &database,
    CObjectVector<CUpdateItem> &updateItems,
    IOutStream *outStream,
    IInStream *inStream,
    CInArchiveInfo *inArchiveInfo,
    CCompressionMethodMode *method,
    CCompressionMethodMode *headerMethod,
    bool useAdditionalHeaderStreams, 
    bool compressMainHeader,
    IArchiveUpdateCallback *updateCallback,
    bool solid,
    bool removeSfxBlock);

}}

#endif
