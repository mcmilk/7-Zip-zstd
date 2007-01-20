// Archive/ArjIn.h

#ifndef __ARCHIVE_ARJIN_H
#define __ARCHIVE_ARJIN_H

#include "Common/MyCom.h"
#include "../../IStream.h"

#include "ArjItem.h"

namespace NArchive {
namespace NArj {
  
class CInArchiveException
{
public:
  enum CCauseType
  {
    kUnexpectedEndOfArchive = 0,
    kCRCError,
    kIncorrectArchive,
    kReadStreamError,
    kSeekStreamError
  } 
  Cause;
  CInArchiveException(CCauseType cause): Cause(cause) {};
};

class CProgressVirt
{
public:
  STDMETHOD(SetCompleted)(const UInt64 *numFiles) PURE;
};

class CInArchive
{
  CMyComPtr<IInStream> _stream;
  UInt64 _streamStartPosition;
  UInt64 _position;
  UInt16 _blockSize;
  Byte _block[kMaxBlockSize];
  UInt32 _blockPos;

  
  bool FindAndReadMarker(const UInt64 *searchHeaderSizeLimit);
  
  bool ReadBlock();
  bool ReadBlock2();

  Byte ReadByte();
  UInt16 ReadUInt16();
  UInt32 ReadUInt32();

  HRESULT ReadBytes(void *data, UInt32 size, UInt32 *processedSize);
  bool ReadBytesAndTestSize(void *data, UInt32 size);
  void SafeReadBytes(void *data, UInt32 size);
  Byte SafeReadByte();
  UInt16 SafeReadUInt16();
  UInt32 SafeReadUInt32();
    
  void IncreasePositionValue(UInt64 addValue);
  void ThrowIncorrectArchiveException();
 
public:
  HRESULT GetNextItem(bool &filled, CItemEx &item);

  bool Open(IInStream *inStream, const UInt64 *searchHeaderSizeLimit);
  void Close();

  void IncreaseRealPosition(UInt64 addValue);
};
  
}}
  
#endif
