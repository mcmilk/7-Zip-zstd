// Archive/GZipIn.h

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
  UInt64 m_StreamStartPosition;
  UInt64 m_Position;
  
  HRESULT ReadBytes(IInStream *inStream, void *data, UInt32 size);
  HRESULT UpdateCRCBytes(IInStream *inStream, UInt32 numBytesToSkeep, CCRC &crc);

  HRESULT ReadZeroTerminatedString(IInStream *inStream, AString &resString);

  HRESULT ReadByte(IInStream *inStream, Byte &value);
  HRESULT ReadUInt16(IInStream *inStream, UInt16 &value);
  HRESULT ReadUInt32(IInStream *inStream, UInt32 &value);
public:
  HRESULT ReadHeader(IInStream *inStream, CItemEx &item);
  HRESULT ReadPostHeader(IInStream *inStream, CItemEx &item);
  UInt64 GetPosition() const { return m_Position; }
};
  
}}
  
#endif
