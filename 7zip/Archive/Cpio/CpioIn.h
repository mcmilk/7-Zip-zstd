// CpioIn.h

#ifndef __ARCHIVE_CPIO_IN_H
#define __ARCHIVE_CPIO_IN_H

#include "Common/MyCom.h"
#include "Common/Types.h"
#include "../../IStream.h"
#include "CpioItem.h"

namespace NArchive {
namespace NCpio {
  
const UInt32 kMaxBlockSize = NFileHeader::kRecordSize;

class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  UInt64 m_Position;

  UInt16 _blockSize;
  Byte _block[kMaxBlockSize];
  UInt32 _blockPos;
  Byte ReadByte();
  UInt16 ReadUInt16();
  UInt32 ReadUInt32();
  
  bool ReadNumber(UInt32 &resultValue);
  bool ReadOctNumber(int size, UInt32 &resultValue);

  HRESULT ReadBytes(void *data, UInt32 size, UInt32 &processedSize);
public:
  HRESULT Open(IInStream *inStream);
  HRESULT GetNextItem(bool &filled, CItemEx &itemInfo);
  HRESULT Skeep(UInt64 numBytes);
  HRESULT SkeepDataRecords(UInt64 dataSize, UInt32 align);
};
  
}}
  
#endif
