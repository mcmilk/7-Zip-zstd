// Archive/TarIn.h

#pragma once

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
  UINT64 m_Position;
  
  HRESULT ReadBytes(void *data, UINT32 size, UINT32 &processedSize);
public:
  HRESULT Open(IInStream *inStream);
  HRESULT GetNextItemReal(bool &filled, CItemEx &itemInfo);
  HRESULT GetNextItem(bool &filled, CItemEx &itemInfo);
  HRESULT Skeep(UINT64 numBytes);
  HRESULT SkeepDataRecords(UINT64 dataSize);
};
  
}}
  
#endif
