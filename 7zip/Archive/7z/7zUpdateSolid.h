// 7z/UpdateSolid.h

#pragma once

#ifndef __7Z_UPDATE_SOLID_H
#define __7Z_UPDATE_SOLID_H

#include "7zOut.h"
#include "7zIn.h"
#include "7zUpdateItem.h"
#include "../IArchive.h"

// #include "Common/Vector.h"
// #include "Common/Types.h"
// #include "Interface/ICoder.h"
// #include "Windows/PropVariant.h"


// #include "7zItem.h"

namespace NArchive {
namespace N7z {

HRESULT UpdateSolidStd(COutArchive &archive, 
    IInStream *inStream,
    const CCompressionMethodMode *method, 
    const CCompressionMethodMode *headerMethod,
    bool useAdditionalHeaderStreams, 
    bool compressMainHeader,
    const CArchiveDatabaseEx &database,
    const CObjectVector<CUpdateItem> &updateItems,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
