// Archive/ArjIn.h

#pragma once

#ifndef __ARCHIVE_ARJIN_H
#define __ARCHIVE_ARJIN_H

#include "Common/Exception.h"
#include "Common/MyCom.h"
#include "../../IStream.h"

#include "Header.h"
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
  CInArchiveException(CCauseType cause);
};

class CProgressVirt
{
public:
  STDMETHOD(SetCompleted)(const UINT64 *numFiles) PURE;
};

class CInArchive
{
  CMyComPtr<IInStream> _stream;
  UINT64 _streamStartPosition;
  UINT64 _position;
  UINT16 _blockSize;
  BYTE _block[kMaxBlockSize];
  
  bool FindAndReadMarker(const UINT64 *searchHeaderSizeLimit);
  
  bool ReadBlock();
  bool ReadBlock2();

  HRESULT ReadBytes(void *data, UINT32 size, UINT32 *processedSize);
  bool ReadBytesAndTestSize(void *data, UINT32 size);
  void SafeReadBytes(void *data, UINT32 size);
  
  void IncreasePositionValue(UINT64 addValue);
  void ThrowIncorrectArchiveException();
 
public:
  HRESULT GetNextItem(bool &filled, CItemEx &item);

  bool Open(IInStream *inStream, const UINT64 *searchHeaderSizeLimit);
  void Close();

  void IncreaseRealPosition(UINT64 addValue);
};
  
}}
  
#endif
