// TarUpdate.h

#ifndef __TAR_UPDATE_H
#define __TAR_UPDATE_H

#include "../IArchive.h"
#include "TarItem.h"

namespace NArchive {
namespace NTar {

struct CUpdateItem
{
  int IndexInArchive;
  int IndexInClient;
  Int64 MTime;
  UInt64 Size;
  UInt32 Mode;
  bool NewData;
  bool NewProps;
  bool IsDir;
  AString Name;
  AString User;
  AString Group;
};

HRESULT UpdateArchive(IInStream *inStream, ISequentialOutStream *outStream,
    const CObjectVector<CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    UINT codePage,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
