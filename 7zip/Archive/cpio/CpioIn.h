// CpioIn.h

#pragma once

#ifndef __ARCHIVE_CPIO_IN_H
#define __ARCHIVE_CPIO_IN_H

#include "Common/MyCom.h"
#include "../../IStream.h"
#include "CpioItem.h"

namespace NArchive {
namespace NCpio {
  
class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  
  UINT64 m_Position;
  
  HRESULT ReadBytes(void *data, UINT32 size, UINT32 &aProcessedSize);
public:
  HRESULT Open(IInStream *inStream);
  HRESULT GetNextItem(bool &filled, CItemEx &anItemInfo);
  HRESULT Skeep(UINT64 aNumBytes);
  HRESULT SkeepDataRecords(UINT64 aDataSize, bool OldHeader);
};
  
}}
  
#endif
