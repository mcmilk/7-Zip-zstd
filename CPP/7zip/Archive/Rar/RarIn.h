// RarIn.h

#ifndef __ARCHIVE_RAR_IN_H
#define __ARCHIVE_RAR_IN_H

#include "Common/DynamicBuffer.h"
#include "Common/MyCom.h"

#include "../../ICoder.h"
#include "../../IStream.h"

#include "../../Common/StreamObjects.h"

#include "../../Crypto/RarAes.h"

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


struct CInArchiveInfo
{
  UInt32 Flags;
  Byte EncryptVersion;
  UInt64 StartPosition;
  
  bool IsSolid() const { return (Flags & NHeader::NArchive::kSolid) != 0; }
  bool IsCommented() const {  return (Flags & NHeader::NArchive::kComment) != 0; }
  bool IsVolume() const {  return (Flags & NHeader::NArchive::kVolume) != 0; }
  bool HaveNewVolumeName() const {  return (Flags & NHeader::NArchive::kNewVolName) != 0; }
  bool IsEncrypted() const { return (Flags & NHeader::NArchive::kBlockEncryption) != 0; }
  bool IsThereEncryptVer() const { return (Flags & NHeader::NArchive::kEncryptVer) != 0; }
  bool IsEncryptOld() const { return (!IsThereEncryptVer() || EncryptVersion < 36); }
};

class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  
  UInt64 m_StreamStartPosition;
  
  CInArchiveInfo _header;
  CDynamicBuffer<char> m_NameBuffer;
  CDynamicBuffer<wchar_t> _unicodeNameBuffer;

  CByteBuffer _comment;
  
  void ReadName(CItemEx &item, int nameSize);
  void ReadHeaderReal(CItemEx &item);
  
  HRESULT ReadBytesSpec(void *data, size_t *size);
  bool ReadBytesAndTestSize(void *data, UInt32 size);
  
  HRESULT Open2(IInStream *stream, const UInt64 *searchHeaderSizeLimit);

  void ThrowExceptionWithCode(CInArchiveException::CCauseType cause);
  void ThrowUnexpectedEndOfArchiveException();
  
  void AddToSeekValue(UInt64 addValue);
  
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
  Byte *m_DecryptedDataAligned;
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

public:
  UInt64 m_Position;

  HRESULT Open(IInStream *inStream, const UInt64 *searchHeaderSizeLimit);
  void Close();
  HRESULT GetNextItem(CItemEx &item, ICryptoGetTextPassword *getTextPassword, bool &decryptionError, AString &errorMessage);
  
  void GetArchiveInfo(CInArchiveInfo &archiveInfo) const;
  
  void SeekInArchive(UInt64 position);
  ISequentialInStream *CreateLimitedStream(UInt64 position, UInt64 size);
};
  
}}
  
#endif
