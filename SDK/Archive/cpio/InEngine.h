// Archive/cpio/InEngine.h

#pragma once

#ifndef __ARCHIVE_cpio_INENGINE_H
#define __ARCHIVE_cpio_INENGINE_H

#include "Archive/cpio/ItemInfoEx.h"
#include "Interface/IInOutStreams.h"

namespace NArchive {
namespace Ncpio {
  
class CInArchive
{
  CComPtr<IInStream> m_Stream;
  
  UINT64 m_Position;
  
  HRESULT ReadBytes(void *aData, UINT32 aSize, UINT32 &aProcessedSize);
public:
  HRESULT Open(IInStream *aStream);
  HRESULT GetNextItem(bool &aFilled, CItemInfoEx &anItemInfo);
  HRESULT Skeep(UINT64 aNumBytes);
  HRESULT SkeepDataRecords(UINT64 aDataSize);
};
  
}}
  
#endif
