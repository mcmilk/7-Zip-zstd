// Tar/UpdateEngine.h

#pragma once

#ifndef __TAR_UPDATEAENGINE_H
#define __TAR_UPDATEAENGINE_H

#include "Common/Vector.h"
#include "Common/Types.h"
#include "Common/String.h"

#include "../Common/ArchiveInterface.h"
#include "Archive/Tar/ItemInfoEx.h"

namespace NArchive {
namespace NTar {

struct CUpdateItemInfo
{
  bool NewData;
  bool NewProperties;
  int IndexInArchive;
  int IndexInClient;

  time_t Time;
  UINT64 Size;
  AString Name;
  bool IsDirectory;
};

HRESULT UpdateArchive(IInStream *inStream, ISequentialOutStream *outStream,
    const NArchive::NTar::CItemInfoExVector &inputItems,
    const CObjectVector<CUpdateItemInfo> &updateItems,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
