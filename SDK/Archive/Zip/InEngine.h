// Archive/Zip/InEngine.h

#pragma once

#ifndef __ZIP_INENGINE_H
#define __ZIP_INENGINE_H

#include "Archive/Zip/Header.h"
#include "Archive/Zip/ItemInfoEx.h"
#include "Common/Exception.h"
#include "Interface/IInOutStreams.h"

namespace NArchive {
namespace NZip {
  
class CInArchiveException: public CCException
{
public:
  enum CCauseType
  {
    kUnexpectedEndOfArchive = 0,
    kArchiceHeaderCRCError,
    kFileHeaderCRCError,
    kIncorrectArchive,
    kDataDescroptorsAreNotSupported,
    kMultiVolumeArchiveAreNotSupported,
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
  UINT32 m_Signature;
  UINT64 m_StreamStartPosition;
  UINT64 m_Position;
  CInArchiveInfo m_ArchiveInfo;
  AString m_NameBuffer;
  
  bool FindAndReadMarker(const UINT64 *aSearchHeaderSizeLimit);
  bool ReadSignature(UINT32 &aSignature);
  AString ReadFileName(UINT32 aNameSize);
  
  HRESULT ReadBytes(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  bool ReadBytesAndTestSize(void *aData, UINT32 aSize);
  void SafeReadBytes(void *aData, UINT32 aSize);
  
  void IncreasePositionValue(UINT64 anAddValue);
  void IncreaseRealPosition(UINT64 anAddValue);
  void ThrowIncorrectArchiveException();
 
public:
  HRESULT ReadHeaders(CItemInfoExVector &anItems, CProgressVirt *aProgress);
  bool Open(IInStream *aStreamm, const UINT64 *aSearchHeaderSizeLimit);
  void Close();
  void GetArchiveInfo(CInArchiveInfo &anArchiveInfo) const;
  void DirectGetBytes(void *aData, UINT32 aNum);
  bool SeekInArchive(UINT64 aPosition);
  ISequentialInStream *CreateLimitedStream(UINT64 aPosition, UINT64 aSize);
  IInStream* CreateStream();
};
  
}}
  
#endif
