// Archive/Deb/InEngine.h

#pragma once

#ifndef __ARCHIVE_DEB_INENGINE_H
#define __ARCHIVE_DEB_INENGINE_H

#include "Archive/Deb/ItemInfoEx.h"
#include "Interface/IInOutStreams.h"

namespace NArchive {
namespace NDeb {
  
class CInArchive
{
  CComPtr<IInStream> m_Stream;
  UINT64 m_Position;
  
  HRESULT ReadBytes(void *data, UINT32 size, UINT32 &processedSize);
  HRESULT GetNextItemReal(bool &filled, CItemInfoEx &itemInfo);
  HRESULT Skeep(UINT64 numBytes);
public:
  HRESULT Open(IInStream *inStream);
  HRESULT GetNextItem(bool &filled, CItemInfoEx &itemInfo);
  HRESULT SkeepData(UINT64 dataSize);
};
  
}}
  
#endif
