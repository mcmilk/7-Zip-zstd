// Archive/DebIn.h

#pragma once

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
  UINT64 m_Position;
  
  HRESULT ReadBytes(void *data, UINT32 size, UINT32 &processedSize);
  HRESULT GetNextItemReal(bool &filled, CItemEx &itemInfo);
  HRESULT Skeep(UINT64 numBytes);
public:
  HRESULT Open(IInStream *inStream);
  HRESULT GetNextItem(bool &filled, CItemEx &itemInfo);
  HRESULT SkeepData(UINT64 dataSize);
};
  
}}
  
#endif
