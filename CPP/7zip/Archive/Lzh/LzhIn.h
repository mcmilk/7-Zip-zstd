// Archive/LzhIn.h

#ifndef __ARCHIVE_LZHIN_H
#define __ARCHIVE_LZHIN_H

#include "Common/MyCom.h"
#include "../../IStream.h"

#include "LzhItem.h"

namespace NArchive {
namespace NLzh {
  
class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  UInt64 m_Position;
  
  HRESULT ReadBytes(void *data, UInt32 size, UInt32 &processedSize);
  HRESULT CheckReadBytes(void *data, UInt32 size);
public:
  HRESULT Open(IInStream *inStream);
  HRESULT GetNextItem(bool &filled, CItemEx &itemInfo);
  HRESULT Skeep(UInt64 numBytes);
};
  
}}
  
#endif
