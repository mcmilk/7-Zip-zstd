// Archive/RarIn.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/UTFConvert.h"

#include "RarIn.h"
#include "../../Common/LimitedStreams.h"
#include "../../Common/StreamUtils.h"

extern "C" 
{ 
  #include "../../../../C/7zCrc.h" 
}

namespace NArchive {
namespace NRar {
 
void CInArchive::ThrowExceptionWithCode(
    CInArchiveException::CCauseType cause)
{
  throw CInArchiveException(cause);
}

bool CInArchive::Open(IInStream *inStream, const UInt64 *searchHeaderSizeLimit)
{
  m_CryptoMode = false;
  if(inStream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition) != S_OK)
    return false;
  m_Position = m_StreamStartPosition;
  m_Stream = inStream;
  if (ReadMarkerAndArchiveHeader(searchHeaderSizeLimit))
    return true;
  m_Stream.Release();
  return false;
}

void CInArchive::Close()
{
  m_Stream.Release();
}


static inline bool TestMarkerCandidate(const void *aTestBytes)
{
  for (UInt32 i = 0; i < NHeader::kMarkerSize; i++)
    if (((const Byte *)aTestBytes)[i] != NHeader::kMarker[i])
      return false;
  return true;
}

bool CInArchive::FindAndReadMarker(const UInt64 *searchHeaderSizeLimit)
{
  // if (m_Length < NHeader::kMarkerSize)
  //   return false;
  m_ArchiveStartPosition = 0;
  m_Position = m_StreamStartPosition;
  if(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL) != S_OK)
    return false;

  Byte marker[NHeader::kMarkerSize];
  UInt32 processedSize; 
  ReadBytes(marker, NHeader::kMarkerSize, &processedSize);
  if(processedSize != NHeader::kMarkerSize)
    return false;
  if (TestMarkerCandidate(marker))
    return true;

  CByteDynamicBuffer dynamicBuffer;
  static const UInt32 kSearchMarkerBufferSize = 0x10000;
  dynamicBuffer.EnsureCapacity(kSearchMarkerBufferSize);
  Byte *buffer = dynamicBuffer;
  UInt32 numBytesPrev = NHeader::kMarkerSize - 1;
  memmove(buffer, marker + 1, numBytesPrev);
  UInt64 curTestPos = m_StreamStartPosition + 1;
  for (;;)
  {
    if (searchHeaderSizeLimit != NULL)
      if (curTestPos - m_StreamStartPosition > *searchHeaderSizeLimit)
        break;
    UInt32 numReadBytes = kSearchMarkerBufferSize - numBytesPrev;
    ReadBytes(buffer + numBytesPrev, numReadBytes, &processedSize);
    UInt32 numBytesInBuffer = numBytesPrev + processedSize;
    if (numBytesInBuffer < NHeader::kMarkerSize)
      break;
    UInt32 numTests = numBytesInBuffer - NHeader::kMarkerSize + 1;
    for(UInt32 pos = 0; pos < numTests; pos++, curTestPos++)
    { 
      if (TestMarkerCandidate(buffer + pos))
      {
        m_ArchiveStartPosition = curTestPos;
        m_Position = curTestPos + NHeader::kMarkerSize;
        if(m_Stream->Seek(m_Position, STREAM_SEEK_SET, NULL) != S_OK)
          return false;
        return true;
      }
    }
    numBytesPrev = numBytesInBuffer - numTests;
    memmove(buffer, buffer + numTests, numBytesPrev);
  }
  return false;
}

void CInArchive::ThrowUnexpectedEndOfArchiveException()
{
  ThrowExceptionWithCode(CInArchiveException::kUnexpectedEndOfArchive);
}

bool CInArchive::ReadBytesAndTestSize(void *data, UInt32 size)
{
  if (m_CryptoMode)
  {
    const Byte *bufData = (const Byte *)m_DecryptedData;
    UInt32 bufSize = m_DecryptedDataSize;
    UInt32 i;
    for (i = 0; i < size && m_CryptoPos < bufSize; i++)
      ((Byte *)data)[i] = bufData[m_CryptoPos++];
    return (i == size);
  }
  UInt32 processedSize;
  ReadStream(m_Stream, data, size, &processedSize);
  return (processedSize == size);
}

void CInArchive::ReadBytesAndTestResult(void *data, UInt32 size)
{
  if(!ReadBytesAndTestSize(data,size))
    ThrowUnexpectedEndOfArchiveException();
}

HRESULT CInArchive::ReadBytes(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  HRESULT result = ReadStream(m_Stream, data, size, &realProcessedSize);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  AddToSeekValue(realProcessedSize);
  return result;
}

static UInt32 CrcUpdateUInt16(UInt32 crc, UInt16 v)
{
  crc = CRC_UPDATE_BYTE(crc, (Byte)(v & 0xFF));
  crc = CRC_UPDATE_BYTE(crc, (Byte)((v >> 8) & 0xFF));
  return crc;
}

static UInt32 CrcUpdateUInt32(UInt32 crc, UInt32 v)
{
  crc = CRC_UPDATE_BYTE(crc, (Byte)(v & 0xFF));
  crc = CRC_UPDATE_BYTE(crc, (Byte)((v >> 8) & 0xFF));
  crc = CRC_UPDATE_BYTE(crc, (Byte)((v >> 16) & 0xFF));
  crc = CRC_UPDATE_BYTE(crc, (Byte)((v >> 24) & 0xFF));
  return crc;
}


bool CInArchive::ReadMarkerAndArchiveHeader(const UInt64 *searchHeaderSizeLimit)
{
  if (!FindAndReadMarker(searchHeaderSizeLimit))
    return false;

  Byte buf[NHeader::NArchive::kArchiveHeaderSize];
  UInt32 processedSize;
  ReadBytes(buf, sizeof(buf), &processedSize);
  if (processedSize != sizeof(buf))
    return false;
  m_CurData = buf;
  m_CurPos  = 0;
  m_PosLimit = sizeof(buf);

  m_ArchiveHeader.CRC = ReadUInt16();
  m_ArchiveHeader.Type = ReadByte();
  m_ArchiveHeader.Flags = ReadUInt16();
  m_ArchiveHeader.Size = ReadUInt16();
  m_ArchiveHeader.Reserved1 = ReadUInt16();
  m_ArchiveHeader.Reserved2 = ReadUInt32();
  m_ArchiveHeader.EncryptVersion = 0;

  UInt32 crc = CRC_INIT_VAL;
  crc = CRC_UPDATE_BYTE(crc, m_ArchiveHeader.Type);
  crc = CrcUpdateUInt16(crc, m_ArchiveHeader.Flags);
  crc = CrcUpdateUInt16(crc, m_ArchiveHeader.Size);
  crc = CrcUpdateUInt16(crc, m_ArchiveHeader.Reserved1);
  crc = CrcUpdateUInt32(crc, m_ArchiveHeader.Reserved2);

  if (m_ArchiveHeader.IsThereEncryptVer() && m_ArchiveHeader.Size > NHeader::NArchive::kArchiveHeaderSize)
  {
    ReadBytes(&m_ArchiveHeader.EncryptVersion, 1, &processedSize);
    if (processedSize != 1)
      return false;
    crc = CRC_UPDATE_BYTE(crc, m_ArchiveHeader.EncryptVersion);
  }

  if(m_ArchiveHeader.CRC != (CRC_GET_DIGEST(crc) & 0xFFFF))
    ThrowExceptionWithCode(CInArchiveException::kArchiveHeaderCRCError);
  if (m_ArchiveHeader.Type != NHeader::NBlockType::kArchiveHeader)
    return false;
  m_ArchiveCommentPosition = m_Position;
  m_SeekOnArchiveComment = true;
  return true;
}

void CInArchive::SkipArchiveComment()
{
  if (!m_SeekOnArchiveComment)
    return;
  AddToSeekValue(m_ArchiveHeader.Size - m_ArchiveHeader.GetBaseSize());
  m_SeekOnArchiveComment = false;
}

void CInArchive::GetArchiveInfo(CInArchiveInfo &archiveInfo) const
{
  archiveInfo.StartPosition = m_ArchiveStartPosition;
  archiveInfo.Flags = m_ArchiveHeader.Flags;
  archiveInfo.CommentPosition = m_ArchiveCommentPosition;
  archiveInfo.CommentSize = (UInt16)(m_ArchiveHeader.Size - NHeader::NArchive::kArchiveHeaderSize);
}

static void DecodeUnicodeFileName(const char *name, const Byte *encName, 
    int encSize, wchar_t *unicodeName, int maxDecSize)
{
  int encPos = 0;
  int decPos = 0;
  int flagBits = 0;
  Byte flags = 0;
  Byte highByte = encName[encPos++];
  while (encPos < encSize && decPos < maxDecSize)
  {
    if (flagBits == 0)
    {
      flags = encName[encPos++];
      flagBits = 8;
    }
    switch(flags >> 6)
    {
      case 0:
        unicodeName[decPos++] = encName[encPos++];
        break;
      case 1:
        unicodeName[decPos++] = (wchar_t)(encName[encPos++] + (highByte << 8));
        break;
      case 2:
        unicodeName[decPos++] = (wchar_t)(encName[encPos] + (encName[encPos + 1] << 8));
        encPos += 2;
        break;
      case 3:
        {
          int length = encName[encPos++];
          if (length & 0x80)
          {
            Byte correction = encName[encPos++];
            for (length = (length & 0x7f) + 2; 
                length > 0 && decPos < maxDecSize; length--, decPos++)
              unicodeName[decPos] = (wchar_t)(((name[decPos] + correction) & 0xff) + (highByte << 8));
          }
          else
            for (length += 2; length > 0 && decPos < maxDecSize; length--, decPos++)
              unicodeName[decPos] = name[decPos];
        }
        break;
    }
    flags <<= 2;
    flagBits -= 2;
  }
  unicodeName[decPos < maxDecSize ? decPos : maxDecSize - 1] = 0;
}

void CInArchive::ReadName(CItemEx &item, int nameSize)
{
  item.UnicodeName.Empty();
  if (nameSize > 0)
  {
    m_NameBuffer.EnsureCapacity(nameSize + 1);
    char *buffer = (char *)m_NameBuffer;

    for (int i = 0; i < nameSize; i++)
      buffer[i] = ReadByte();

    int mainLen;
    for (mainLen = 0; mainLen < nameSize; mainLen++)
      if (buffer[mainLen] == '\0')
        break;
    buffer[mainLen] = '\0';
    item.Name = buffer;

    if(item.HasUnicodeName())
    {
      if(mainLen < nameSize)
      {
        int unicodeNameSizeMax = MyMin(nameSize, (0x400));
        _unicodeNameBuffer.EnsureCapacity(unicodeNameSizeMax + 1);
        DecodeUnicodeFileName(buffer, (const Byte *)buffer + mainLen + 1, 
            nameSize - (mainLen + 1), _unicodeNameBuffer, unicodeNameSizeMax);
        item.UnicodeName = _unicodeNameBuffer;
      }
      else if (!ConvertUTF8ToUnicode(item.Name, item.UnicodeName))
        item.UnicodeName.Empty();
    }
  }
  else
    item.Name.Empty();
}

Byte CInArchive::ReadByte()
{
  if (m_CurPos >= m_PosLimit)
    throw CInArchiveException(CInArchiveException::kIncorrectArchive);
  return m_CurData[m_CurPos++];
}

UInt16 CInArchive::ReadUInt16()
{
  UInt16 value = 0;
  for (int i = 0; i < 2; i++)
  {
    Byte b = ReadByte();
    value |= (UInt16(b) << (8 * i));
  }
  return value;
}

UInt32 CInArchive::ReadUInt32()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
  {
    Byte b = ReadByte();
    value |= (UInt32(b) << (8 * i));
  }
  return value;
}

void CInArchive::ReadTime(Byte mask, CRarTime &rarTime)
{
  rarTime.LowSecond = (Byte)(((mask & 4) != 0) ? 1 : 0);
  int numDigits = (mask & 3);
  rarTime.SubTime[0] = rarTime.SubTime[1] = rarTime.SubTime[2] = 0;
  for (int i = 0; i < numDigits; i++)
    rarTime.SubTime[3 - numDigits + i] = ReadByte();
}

void CInArchive::ReadHeaderReal(CItemEx &item)
{
  item.Flags = m_BlockHeader.Flags; 
  item.PackSize = ReadUInt32();
  item.UnPackSize = ReadUInt32();
  item.HostOS = ReadByte();
  item.FileCRC = ReadUInt32();
  item.LastWriteTime.DosTime = ReadUInt32();
  item.UnPackVersion = ReadByte();
  item.Method = ReadByte();
  int nameSize = ReadUInt16();
  item.Attributes = ReadUInt32();

  item.LastWriteTime.LowSecond = 0;
  item.LastWriteTime.SubTime[0] = 
      item.LastWriteTime.SubTime[1] = 
      item.LastWriteTime.SubTime[2] = 0;

  if((item.Flags & NHeader::NFile::kSize64Bits) != 0)
  {
    item.PackSize |= ((UInt64)ReadUInt32() << 32);
    item.UnPackSize |= ((UInt64)ReadUInt32() << 32);
  }

  ReadName(item, nameSize);

  if (item.HasSalt())
    for (int i = 0; i < sizeof(item.Salt); i++)
      item.Salt[i] = ReadByte();

  // some rar archives have HasExtTime flag without field.
  if (m_CurPos < m_PosLimit && item.HasExtTime())
  {
    Byte accessMask = (Byte)(ReadByte() >> 4);
    Byte b = ReadByte();
    Byte modifMask = (Byte)(b >> 4);
    Byte createMask = (Byte)(b & 0xF);
    if ((modifMask & 8) != 0)
      ReadTime(modifMask, item.LastWriteTime);
    item.IsCreationTimeDefined = ((createMask & 8) != 0);
    if (item.IsCreationTimeDefined)
    {
      item.CreationTime.DosTime = ReadUInt32();
      ReadTime(createMask, item.CreationTime);
    }
    item.IsLastAccessTimeDefined = ((accessMask & 8) != 0);
    if (item.IsLastAccessTimeDefined)
    {
      item.LastAccessTime.DosTime = ReadUInt32();
      ReadTime(accessMask, item.LastAccessTime);
    }
  }

  UInt16 fileHeaderWithNameSize = (UInt16)m_CurPos;
  
  item.Position = m_Position;
  item.MainPartSize = fileHeaderWithNameSize;
  item.CommentSize = (UInt16)(m_BlockHeader.HeadSize - fileHeaderWithNameSize);

  if (m_CryptoMode)
    item.AlignSize = (UInt16)((16 - ((m_BlockHeader.HeadSize) & 0xF)) & 0xF);
  else
    item.AlignSize = 0;
  AddToSeekValue(m_BlockHeader.HeadSize);
}

void CInArchive::AddToSeekValue(UInt64 addValue)
{
  m_Position += addValue;
}

HRESULT CInArchive::GetNextItem(CItemEx &item, ICryptoGetTextPassword *getTextPassword)
{
  if (m_SeekOnArchiveComment)
    SkipArchiveComment();
  for (;;)
  {
    if(!SeekInArchive(m_Position))
      return S_FALSE;
    if (!m_CryptoMode && (m_ArchiveHeader.Flags & 
        NHeader::NArchive::kBlockHeadersAreEncrypted) != 0)
    {
      m_CryptoMode = false;
      if (getTextPassword == 0)
        return S_FALSE;
      if(!SeekInArchive(m_Position))
        return S_FALSE;
      if (!m_RarAES)
      {
        m_RarAESSpec = new NCrypto::NRar29::CDecoder;
        m_RarAES = m_RarAESSpec;
      }
      m_RarAESSpec->SetRar350Mode(m_ArchiveHeader.IsEncryptOld());

      // Salt
      const UInt32 kSaltSize = 8;
      Byte salt[kSaltSize];
      if(!ReadBytesAndTestSize(salt, kSaltSize))
        return S_FALSE;
      m_Position += kSaltSize;
      RINOK(m_RarAESSpec->SetDecoderProperties2(salt, kSaltSize))
      // Password
      CMyComBSTR password;
      RINOK(getTextPassword->CryptoGetTextPassword(&password))
      UString unicodePassword(password);

      CByteBuffer buffer;
      const UInt32 sizeInBytes = unicodePassword.Length() * 2;
      buffer.SetCapacity(sizeInBytes);
      for (int i = 0; i < unicodePassword.Length(); i++)
      {
        wchar_t c = unicodePassword[i];
        ((Byte *)buffer)[i * 2] = (Byte)c;
        ((Byte *)buffer)[i * 2 + 1] = (Byte)(c >> 8);
      }

      RINOK(m_RarAESSpec->CryptoSetPassword((const Byte *)buffer, sizeInBytes));

      const UInt32 kDecryptedBufferSize = (1 << 12);
      if (m_DecryptedData.GetCapacity() == 0)
      {
        m_DecryptedData.SetCapacity(kDecryptedBufferSize);
      }
      RINOK(m_RarAES->Init());
      RINOK(ReadStream(m_Stream, (Byte *)m_DecryptedData, kDecryptedBufferSize, &m_DecryptedDataSize));
      m_DecryptedDataSize = m_RarAES->Filter((Byte *)m_DecryptedData, m_DecryptedDataSize);

      m_CryptoMode = true;
      m_CryptoPos = 0;
    }

    m_FileHeaderData.EnsureCapacity(7);
    if(!ReadBytesAndTestSize((Byte *)m_FileHeaderData, 7))
      return S_FALSE;

    m_CurData = (Byte *)m_FileHeaderData;
    m_CurPos = 0;
    m_PosLimit = 7;
    m_BlockHeader.CRC = ReadUInt16();
    m_BlockHeader.Type = ReadByte();
    m_BlockHeader.Flags = ReadUInt16();
    m_BlockHeader.HeadSize = ReadUInt16();

    if (m_BlockHeader.HeadSize < 7)
      ThrowExceptionWithCode(CInArchiveException::kIncorrectArchive);

    if (m_BlockHeader.Type == NHeader::NBlockType::kEndOfArchive) 
      return S_FALSE;

    if (m_BlockHeader.Type == NHeader::NBlockType::kFileHeader) 
    {
      m_FileHeaderData.EnsureCapacity(m_BlockHeader.HeadSize);
      m_CurData = (Byte *)m_FileHeaderData;
      m_PosLimit = m_BlockHeader.HeadSize;
      ReadBytesAndTestResult(m_CurData + m_CurPos, m_BlockHeader.HeadSize - 7);
      ReadHeaderReal(item); 
      if ((CrcCalc(m_CurData + 2, 
          m_BlockHeader.HeadSize - item.CommentSize - 2) & 0xFFFF) != m_BlockHeader.CRC)
        ThrowExceptionWithCode(CInArchiveException::kFileHeaderCRCError);

      FinishCryptoBlock();
      m_CryptoMode = false;
      SeekInArchive(m_Position); // Move Position to compressed Data;    
      AddToSeekValue(item.PackSize);  // m_Position points to next header;
      return S_OK;
    }
    if (m_CryptoMode && m_BlockHeader.HeadSize > (1 << 12))
      return E_FAIL; // it's for bad passwords
    if ((m_BlockHeader.Flags & NHeader::NBlock::kLongBlock) != 0)
    {
      m_FileHeaderData.EnsureCapacity(7 + 4);
      m_CurData = (Byte *)m_FileHeaderData;
      ReadBytesAndTestResult(m_CurData + m_CurPos, 4); // test it
      m_PosLimit = 7 + 4;
      UInt32 dataSize = ReadUInt32();
      AddToSeekValue(dataSize);
      if (m_CryptoMode && dataSize > (1 << 27))
        return E_FAIL; // it's for bad passwords
      m_CryptoPos = m_BlockHeader.HeadSize;
    }
    else
      m_CryptoPos = 0;
    AddToSeekValue(m_BlockHeader.HeadSize);
    FinishCryptoBlock();
    m_CryptoMode = false;
  }
}

void CInArchive::DirectGetBytes(void *data, UInt32 size)
{  
  ReadStream(m_Stream, data, size, NULL);
}

bool CInArchive::SeekInArchive(UInt64 position)
{
  UInt64 newPosition;
  m_Stream->Seek(position, STREAM_SEEK_SET, &newPosition);
  return newPosition == position;
}

ISequentialInStream* CInArchive::CreateLimitedStream(UInt64 position, UInt64 size)
{
  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  SeekInArchive(position);
  streamSpec->SetStream(m_Stream);
  streamSpec->Init(size);
  return inStream.Detach();
}

}}
