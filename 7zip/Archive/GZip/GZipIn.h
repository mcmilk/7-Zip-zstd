// Archive/GZipIn.h

#pragma once

#ifndef __ARCHIVE_GZIP_IN_H
#define __ARCHIVE_GZIP_IN_H

#include "GZipHeader.h"
#include "GZipItem.h"
#include "../../IStream.h"
#include "Common/CRC.h"

namespace NArchive {
namespace NGZip {
  
class CInArchive
{
  UINT64 m_StreamStartPosition;
  UINT64 m_Position;
  
  HRESULT ReadBytes(IInStream *inStream, void *data, UINT32 size);
  HRESULT UpdateCRCBytes(IInStream *inStream, UINT32 numBytesToSkeep, CCRC &crc);

  HRESULT ReadZeroTerminatedString(IInStream *inStream, AString &resString);

public:
  HRESULT ReadHeader(IInStream *inStream, CItemEx &item);
  HRESULT ReadPostInfo(IInStream *inStream, UINT32 &crc, UINT32 &unpackSize32);
  UINT64 GetPosition() const { return m_Position; }
};
  
}}
  
#endif
