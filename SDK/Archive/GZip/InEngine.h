// Archive/GZip/InEngine.h

#pragma once

#ifndef __ARCHIVE_GZIP_INENGINE_H
#define __ARCHIVE_GZIP_INENGINE_H

#include "Archive/GZip/Header.h"
#include "Archive/GZip/ItemInfoEx.h"
#include "Common/Exception.h"
#include "Interface/IInOutStreams.h"
#include "Common/CRC.h"

namespace NArchive {
namespace NGZip {
  
class CInArchive
{
  UINT64 m_StreamStartPosition;
  UINT64 m_Position;
  
  HRESULT ReadBytes(IInStream *aStream, void *aData, UINT32 aSize);
  HRESULT UpdateCRCBytes(IInStream *aStream, UINT32 anNumBytesToSkeep, CCRC &aCRC);

  HRESULT ReadZeroTerminatedString(IInStream *aStream, AString &aString);

public:
  HRESULT ReadHeader(IInStream *aStream, CItemInfoEx &anItemInfo);
  HRESULT ReadPostInfo(IInStream *aStream, UINT32 &aCRC, UINT32 &anUnpackSize32);
  UINT64 GetPosition() const { return m_Position; }
};
  
}}
  
#endif
