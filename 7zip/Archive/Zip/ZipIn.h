// Archive/ZipIn.h

#pragma once

#ifndef __ZIP_IN_H
#define __ZIP_IN_H

#include "Common/MyCom.h"
#include "../../IStream.h"

#include "ZipHeader.h"
#include "ZipItemEx.h"

namespace NArchive {
namespace NZip {
  
class CInArchiveException
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
  CInArchiveException(CCauseType cause): Cause(cause) {}
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
  STDMETHOD(SetCompleted)(const UINT64 *numFiles) PURE;
};

class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  UINT32 m_Signature;
  UINT64 m_StreamStartPosition;
  UINT64 m_Position;
  CInArchiveInfo m_ArchiveInfo;
  AString m_NameBuffer;
  
  bool FindAndReadMarker(const UINT64 *searchHeaderSizeLimit);
  bool ReadSignature(UINT32 &signature);
  AString ReadFileName(UINT32 nameSize);
  
  HRESULT ReadBytes(void *data, UINT32 size, UINT32 *processedSize);
  bool ReadBytesAndTestSize(void *data, UINT32 size);
  void SafeReadBytes(void *data, UINT32 size);
  
  void IncreasePositionValue(UINT64 addValue);
  void IncreaseRealPosition(UINT64 addValue);
  void ThrowIncorrectArchiveException();
 
public:
  HRESULT ReadHeaders(CObjectVector<CItemEx> &items, CProgressVirt *progress);
  bool Open(IInStream *inStream, const UINT64 *searchHeaderSizeLimit);
  void Close();
  void GetArchiveInfo(CInArchiveInfo &archiveInfo) const;
  void DirectGetBytes(void *data, UINT32 num);
  bool SeekInArchive(UINT64 position);
  ISequentialInStream *CreateLimitedStream(UINT64 position, UINT64 size);
  IInStream* CreateStream();
};
  
}}
  
#endif
