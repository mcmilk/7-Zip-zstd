// Tar/UpdateEngine.h

#pragma once

#ifndef __TAR_UPDATEAENGINE_H
#define __TAR_UPDATEAENGINE_H

#include "Common/Vector.h"
#include "Common/Types.h"
#include "Common/String.h"

#include "../Common/IArchiveHandler.h"
#include "Archive/Tar/ItemInfoEx.h"

namespace NArchive {
namespace NTar {

struct CUpdateItemInfo
{
  int IndexInClient;
  time_t Time;
  UINT64 Size;
  AString Name;
  bool IsDirectory;
};

HRESULT UpdateArchive(IInStream *anInStream, ISequentialOutStream *anOutStream,
    const NArchive::NTar::CItemInfoExVector &anInputItems,
    const CRecordVector<bool> &aCompressStatuses,
    const CObjectVector<CUpdateItemInfo> &anUpdateItems,
    const CRecordVector<UINT32> &aCopyIndexes,
    IUpdateCallBack *anUpdateCallBack);

}}

#endif
