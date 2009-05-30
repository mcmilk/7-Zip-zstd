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
  UInt32 Time;
  UInt32 Mode;
  UInt64 Size;
  AString Name;
  AString User;
  AString Group;
  bool NewData;
  bool NewProps;
  bool IsDir;
};

HRESULT UpdateArchive(IInStream *inStream, ISequentialOutStream *outStream,
    const CObjectVector<CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
