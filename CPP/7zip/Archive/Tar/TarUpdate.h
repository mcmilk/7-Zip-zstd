// Tar/Update.h

#ifndef __TAR_UPDATE_H
#define __TAR_UPDATE_H

#include "../IArchive.h"
#include "TarItem.h"

namespace NArchive {
namespace NTar {

struct CUpdateItem
{
  bool NewData;
  bool NewProperties;
  int IndexInArchive;
  int IndexInClient;

  UInt32 Time;
  UInt64 Size;
  AString Name;
  bool IsDir;
};

HRESULT UpdateArchive(IInStream *inStream, ISequentialOutStream *outStream,
    const CObjectVector<CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
