// 7z/UpdateSolidEngine.h

#pragma once

#ifndef __7Z_UPDATEASOLIDENGINE_H
#define __7Z_UPDATEAENGINE_H

#include "Common/Vector.h"
#include "Common/Types.h"

#include "InEngine.h"
#include "OutEngine.h"
#include "ItemInfo.h"
#include "MethodInfo.h"

#include "../Common/IArchiveHandler.h"
#include "CompressionMethod.h"
#include "Interface/ICoder.h"
#include "Windows/PropVariant.h"
#include "UpdateItemInfo.h"

namespace NArchive {
namespace N7z {

HRESULT UpdateSolidStd(NArchive::N7z::COutArchive &anArchive, 
    IInStream *anInStream,
    const CCompressionMethodMode *aMethod, 
    const CCompressionMethodMode *aHeaderMethod, 
    const NArchive::N7z::CArchiveDatabaseEx &aDatabase,
    const CRecordVector<bool> &aCompressStatuses,
    const CObjectVector<CUpdateItemInfo> &anUpdateItems,
    const CRecordVector<UINT32> &aCopyIndexes,
    IUpdateCallBack *anUpdateCallBack);

}}

#endif
