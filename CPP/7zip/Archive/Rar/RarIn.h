// RarIn.h

#ifndef __ARCHIVE_RAR_IN_H
#define __ARCHIVE_RAR_IN_H

#include "Common/DynamicBuffer.h"
#include "Common/MyCom.h"
#include "../../IStream.h"
#include "../../ICoder.h"
#include "../../Common/StreamObjects.h"
#include "../../Crypto/RarAES/RarAES.h"
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
    kIncorrectArchive
  } 
  Cause;
  CInArchiveException(CCauseType cause) :   Cause(cause) {}
};

class CInArchiveInfo
{
public:
  UInt64 StartPosition;
  UInt16 Flags;
  UInt64 CommentPosition;
  UInt16 CommentSize;
  bool IsSolid() const { return (Flags & NHeader::NArchive::kSolid) != 0; }
  bool IsCommented() const {  return (Flags & NHeader::NArchive::kComment) != 0; }
  bool IsVolume() const {  return (Flags & NHeader::NArchive::kVolume) != 0; }
  bool HaveNewVolumeName() const {  return (Flags & NHeader::NArchive::kNewVolName) != 0; }
  bool IsEncrypted() const { return (Flags & NHeader::NArchive::kBlockEncryption) != 0; }
};

class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  
  UInt64 m_StreamStartPosition;
  UInt64 m_Position;
  UInt64 m_ArchiveStartPosition;
  
  NHeader::NArchive::CHeader360 m_ArchiveHeader;
  CDynamicBuffer<char> m_NameBuffer;
  CDynamicBuffer<wchar_t> _unicodeNameBuffer;
  bool m_SeekOnArchiveComment;
  UInt64 m_ArchiveCommentPosition;
  
  void ReadName(CItemEx &item, int nameSize);
  void ReadHeaderReal(CItemEx &item);
  
  HRESULT ReadBytes(void *data, UInt32 size, UInt32 *aProcessedSize);
  bool ReadBytesAndTestSize(void *data, UInt32 size);
  void ReadBytesAndTestResult(void *data, UInt32 size);
  
  bool FindAndReadMarker(const UInt64 *searchHeaderSizeLimit);
  void ThrowExceptionWithCode(CInArchiveException::CCauseType cause);
  void ThrowUnexpectedEndOfArchiveException();
  
  void AddToSeekValue(UInt64 addValue);
  
protected:

  CDynamicBuffer<Byte> m_FileHeaderData;
  
  NHeader::NBlock::CBlock m_BlockHeader;

  NCrypto::NRar29::CDecoder *m_RarAESSpec;
  CMyComPtr<ICompressFilter> m_RarAES;
  
  Byte *m_CurData; // it must point to start of Rar::Block
  UInt32 m_CurPos;
  UInt32 m_PosLimit;
  Byte ReadByte();
  UInt16 ReadUInt16();
  UInt32 ReadUInt32();
  void ReadTime(Byte mask, CRarTime &rarTime);

  CBuffer<Byte> m_DecryptedData;
  UInt32 m_DecryptedDataSize;

  bool m_CryptoMode;
  UInt32 m_CryptoPos;
  void FinishCryptoBlock()
  {
    if (m_CryptoMode)
      while ((m_CryptoPos & 0xF) != 0)
      {
        m_CryptoPos++;
        m_Position++;
      }
  }

  bool ReadMarkerAndArchiveHeader(const UInt64 *searchHeaderSizeLimit);
public:
  bool Open(IInStream *inStream, const UInt64 *searchHeaderSizeLimit);
  void Close();
  HRESULT GetNextItem(CItemEx &item, ICryptoGetTextPassword *getTextPassword);
  
  void SkipArchiveComment();
  
  void GetArchiveInfo(CInArchiveInfo &archiveInfo) const;
  
  void DirectGetBytes(void *data, UInt32 size);
  
  bool SeekInArchive(UInt64 position);
  ISequentialInStream *CreateLimitedStream(UInt64 position, UInt64 size);
};
  
}}
  
#endif
