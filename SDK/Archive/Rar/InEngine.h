// Archive/Rar/InEngine.h

#pragma once

#ifndef __ARCHIVE_RAR_INENGINE_H
#define __ARCHIVE_RAR_INENGINE_H

#include "Common/DynamicBuffer.h"
#include "Interface/IInOutStreams.h"
#include "Common/Exception.h"
#include "Archive/Rar/Header.h"
#include "Archive/Rar/ItemInfoEx.h"

namespace NArchive {
namespace NRar {

class CInArchiveException: public CCException
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
  CInArchiveException(CCauseType aCause);
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
  CComPtr<IInStream> m_Stream;
  
  UINT64 m_StreamStartPosition;
  UINT64 m_Position;
  UINT64 m_ArchiveStartPosition;
  
  NHeader::NArchive::CBlock m_ArchiveHeader;
  CDynamicBuffer<char> m_NameBuffer;
  CDynamicBuffer<wchar_t> _unicodeNameBuffer;
  bool m_SeekOnArchiveComment;
  UINT64 m_ArchiveCommentPosition;
  
  void CInArchive::ReadName(CItemInfoEx &anItemInfo);
  void ReadHeader32Real(CItemInfoEx &anItemInfo);
  void ReadHeader64Real(CItemInfoEx &anItemInfo);
  
  HRESULT ReadBytes(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  bool ReadBytesAndTestSize(void *aData, UINT32 aSize);
  void ReadBytesAndTestResult(void *aData, UINT32 aSize);
  
  bool FindAndReadMarker(const UINT64 *aSearchHeaderSizeLimit);
  void ThrowExceptionWithCode(CInArchiveException::CCauseType anCause);
  void ThrowUnexpectedEndOfArchiveException();
  
  void AddToSeekValue(UINT64 anAddValue);
  
protected:
  union
  {
    NHeader::NBlock::CBlock m_BlockHeader;
    NHeader::NFile::CBlock32 m_FileHeader32;
    NHeader::NFile::CBlock64 m_FileHeader64;
  };
  
  bool ReadMarkerAndArchiveHeader(const UINT64 *aSearchHeaderSizeLimit);
public:
  bool Open(IInStream *aStream, const UINT64 *aSearchHeaderSizeLimit);
  void Close();
  bool GetNextItem(CItemInfoEx &anItemInfo);
  
  void SkipArchiveComment();
  
  void GetArchiveInfo(CInArchiveInfo &anArchiveInfo) const;
  
  void DirectGetBytes(void *aData, UINT32 aNum);
  
  bool SeekInArchive(UINT64 aPosition);
  ISequentialInStream *CreateLimitedStream(UINT64 aPosition, UINT64 aSize);
};
  
}}
  
#endif
