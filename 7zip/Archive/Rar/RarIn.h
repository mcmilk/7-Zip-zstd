// RarIn.h

#pragma once

#ifndef __ARCHIVE_RAR_IN_H
#define __ARCHIVE_RAR_IN_H

#include "Common/DynamicBuffer.h"
#include "Common/Exception.h"
#include "Common/MyCom.h"
#include "../../IStream.h"
#include "RarHeader.h"
#include "RarItem.h"

namespace NArchive {
namespace NRar {

class CInArchiveException
{
public:
  enum CCauseType
  {
    kUnexpectedEndOfArchive = 0,
    kArchiveHeaderCRCError,
    kFileHeaderCRCError,
    kIncorrectArchive,
  } 
  Cause;
  CInArchiveException(CCauseType cause) :   Cause(cause) {}
};

class CInArchiveInfo
{
public:
  UINT64 StartPosition;
  WORD Flags;
  UINT64 CommentPosition;
  WORD CommentSize;
  bool IsSolid() const { return (Flags & NHeader::NArchive::kSolid) != 0; }
  bool IsCommented() const {  return (Flags & NHeader::NArchive::kComment) != 0; }
  bool IsVolume() const {  return (Flags & NHeader::NArchive::kVolume) != 0; }
  bool HaveNewVolumeName() const {  return (Flags & NHeader::NArchive::kNewVolName) != 0; }
};

class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  
  UINT64 m_StreamStartPosition;
  UINT64 m_Position;
  UINT64 m_ArchiveStartPosition;
  
  NHeader::NArchive::CBlock m_ArchiveHeader;
  CDynamicBuffer<char> m_NameBuffer;
  CDynamicBuffer<wchar_t> _unicodeNameBuffer;
  bool m_SeekOnArchiveComment;
  UINT64 m_ArchiveCommentPosition;
  
  void ReadName(const BYTE *data, CItemEx &item, int nameSize);
  void ReadHeaderReal(const BYTE *data, CItemEx &item);
  
  HRESULT ReadBytes(void *data, UINT32 size, UINT32 *aProcessedSize);
  bool ReadBytesAndTestSize(void *data, UINT32 size);
  void ReadBytesAndTestResult(void *data, UINT32 size);
  
  bool FindAndReadMarker(const UINT64 *searchHeaderSizeLimit);
  void ThrowExceptionWithCode(CInArchiveException::CCauseType cause);
  void ThrowUnexpectedEndOfArchiveException();
  
  void AddToSeekValue(UINT64 addValue);
  
protected:

  CDynamicBuffer<BYTE> m_FileHeaderData;

  NHeader::NBlock::CBlock m_BlockHeader;
  
  bool ReadMarkerAndArchiveHeader(const UINT64 *searchHeaderSizeLimit);
public:
  bool Open(IInStream *inStream, const UINT64 *searchHeaderSizeLimit);
  void Close();
  bool GetNextItem(CItemEx &item);
  
  void SkipArchiveComment();
  
  void GetArchiveInfo(CInArchiveInfo &archiveInfo) const;
  
  void DirectGetBytes(void *data, UINT32 size);
  
  bool SeekInArchive(UINT64 position);
  ISequentialInStream *CreateLimitedStream(UINT64 position, UINT64 size);
};
  
}}
  
#endif
