// Archive/TarIn.h

#ifndef __ARCHIVE_TAR_IN_H
#define __ARCHIVE_TAR_IN_H

#include "Common/MyCom.h"
#include "../../IStream.h"

#include "TarItem.h"

namespace NArchive {
namespace NTar {
  
class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  UInt64 m_Position;
  
  HRESULT ReadBytes(void *data, UInt32 size, UInt32 &processedSize);
public:
  HRESULT Open(IInStream *inStream);
  HRESULT GetNextItemReal(bool &filled, CItemEx &itemInfo);
  HRESULT GetNextItem(bool &filled, CItemEx &itemInfo);
  HRESULT Skeep(UInt64 numBytes);
  HRESULT SkeepDataRecords(UInt64 dataSize);
};
  
}}
  
#endif
