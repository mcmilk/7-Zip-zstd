// Archive/arj/InEngine.h

#pragma once

#ifndef __ARJ_INENGINE_H
#define __ARJ_INENGINE_H

#include "Header.h"
#include "ItemInfoEx.h"
#include "Common/Exception.h"
#include "Interface/IInOutStreams.h"

namespace NArchive {
namespace Narj {
  
class CInArchiveException: public CCException
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
  CInArchiveException(CCauseType aCause);
};

class CInArchiveInfo
{
public:
  UINT64 StartPosition;
  UINT64 CommentPosition;
  UINT16 CommentSize;
  bool IsCommented() const { return (CommentSize != 0); };
};

class CProgressVirt
{
public:
  STDMETHOD(SetCompleted)(const UINT64 *aNumFiles) PURE;
};

class CInArchive
{
  CComPtr<IInStream> m_Stream;
  UINT64 m_StreamStartPosition;
  UINT64 m_Position;

  // UINT16 m_HeaderSize;

  UINT16 m_BlockSize;
  BYTE m_Block[kMaxBlockSize];
  
  bool FindAndReadMarker(const UINT64 *aSearchHeaderSizeLimit);
  
  bool ReadBlock();
  bool ReadBlock2();

  HRESULT ReadBytes(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  bool ReadBytesAndTestSize(void *aData, UINT32 aSize);
  void SafeReadBytes(void *aData, UINT32 aSize);
  
  void IncreasePositionValue(UINT64 anAddValue);
  void ThrowIncorrectArchiveException();
 
public:
  HRESULT GetNextItem(bool &aFilled, CItemInfoEx &anItem);

  bool Open(IInStream *aStreamm, const UINT64 *aSearchHeaderSizeLimit);
  void Close();

  void IncreaseRealPosition(UINT64 anAddValue);

  /*
  void GetArchiveInfo(CInArchiveInfo &anArchiveInfo) const;
  void DirectGetBytes(void *aData, UINT32 aNum);
  bool SeekInArchive(UINT64 aPosition);
  ISequentialInStream *CreateLimitedStream(UINT64 aPosition, UINT64 aSize);
  IInStream* CreateStream();
  */
};
  
}}
  
#endif
