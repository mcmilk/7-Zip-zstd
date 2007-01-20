// Archive/DebIn.h

#ifndef __ARCHIVE_DEB_IN_H
#define __ARCHIVE_DEB_IN_H

#include "Common/MyCom.h"
#include "../../IStream.h"
#include "DebItem.h"

namespace NArchive {
namespace NDeb {
  
class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  UInt64 m_Position;
  
  HRESULT ReadBytes(void *data, UInt32 size, UInt32 &processedSize);
  HRESULT GetNextItemReal(bool &filled, CItemEx &itemInfo);
  HRESULT Skeep(UInt64 numBytes);
public:
  HRESULT Open(IInStream *inStream);
  HRESULT GetNextItem(bool &filled, CItemEx &itemInfo);
  HRESULT SkeepData(UInt64 dataSize);
};
  
}}
  
#endif
