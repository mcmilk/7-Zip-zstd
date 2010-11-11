// Archive/RarIn.cpp

#include "StdAfx.h"

#include "../../../../C/7zCrc.h"
#include "../../../../C/CpuArch.h"

#include "Common/StringConvert.h"
#include "Common/UTFConvert.h"

#include "../../Common/LimitedStreams.h"
#include "../../Common/StreamUtils.h"

#include "../Common/FindSignature.h"

#include "RarIn.h"

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

namespace NArchive {
namespace NRar {

static const char *k_UnexpectedEnd = "Unexpected end of archive";
static const char *k_DecryptionError = "Decryption Error";

void CInArchive::ThrowExceptionWithCode(
    CInArchiveException::CCauseType cause)
{
  throw CInArchiveException(cause);
}

HRESULT CInArchive::Open(IInStream *inStream, const UInt64 *searchHeaderSizeLimit)
{
  try
  {
    Close();
    HRESULT res = Open2(inStream, searchHeaderSizeLimit);
    if (res == S_OK)
      return res;
    Close();
    return res;
  }
  catch(...) { Close(); throw; }
}

void CInArchive::Close()
{
  m_Stream.Release();
}

HRESULT CInArchive::ReadBytesSpec(void *data, size_t *resSize)
{
  if (m_CryptoMode)
  {
    size_t size = *resSize;
    *resSize = 0;
    const Byte *bufData = m_DecryptedDataAligned;
    UInt32 bufSize = m_DecryptedDataSize;
    size_t i;
    for (i = 0; i < size && m_CryptoPos < bufSize; i++)
      ((Byte *)data)[i] = bufData[m_CryptoPos++];
    *resSize = i;
    return S_OK;
  }
  return ReadStream(m_Stream, data, resSize);
}

bool CInArchive::ReadBytesAndTestSize(void *data, UInt32 size)
{
  size_t processed = size;
  if (ReadBytesSpec(data, &processed) != S_OK)
    return false;
  return processed == size;
}

HRESULT CInArchive::Open2(IInStream *stream, const UInt64 *searchHeaderSizeLimit)
{
  m_CryptoMode = false;
  RINOK(stream->Seek(0, STREAM_SEEK_SET, &m_StreamStartPosition));
  m_Position = m_StreamStartPosition;

  UInt64 arcStartPos;
  RINOK(FindSignatureInStream(stream, NHeader::kMarker, NHeader::kMarkerSize,
      searchHeaderSizeLimit, arcStartPos));
  m_Position = arcStartPos + NHeader::kMarkerSize;
  RINOK(stream->Seek(m_Position, STREAM_SEEK_SET, NULL));
  Byte buf[NHeader::NArchive::kArchiveHeaderSize + 1];

  RINOK(ReadStream_FALSE(stream, buf, NHeader::NArchive::kArchiveHeaderSize));
  AddToSeekValue(NHeader::NArchive::kArchiveHeaderSize);


  UInt32 blockSize = Get16(buf + 5);

  _header.EncryptVersion = 0;
  _header.Flags = Get16(buf + 3);

  UInt32 headerSize = NHeader::NArchive::kArchiveHeaderSize;
  if (_header.IsThereEncryptVer())
  {
    if (blockSize <= headerSize)
      return S_FALSE;
    RINOK(ReadStream_FALSE(stream, buf + NHeader::NArchive::kArchiveHeaderSize, 1));
    AddToSeekValue(1);
    _header.EncryptVersion = buf[NHeader::NArchive::kArchiveHeaderSize];
    headerSize += 1;
  }
  if (blockSize < headerSize ||
      buf[2] != NHeader::NBlockType::kArchiveHeader ||
      (UInt32)Get16(buf) != (CrcCalc(buf + 2, headerSize - 2) & 0xFFFF))
    return S_FALSE;

  size_t commentSize = blockSize - headerSize; 
  _comment.SetCapacity(commentSize);
  RINOK(ReadStream_FALSE(stream, _comment, commentSize));
  AddToSeekValue(commentSize);
  m_Stream = stream;
  _header.StartPosition = arcStartPos;
  return S_OK;
}

void CInArchive::GetArchiveInfo(CInArchiveInfo &archiveInfo) const
{
  archiveInfo = _header;
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
  item.Size = ReadUInt32();
  item.HostOS = ReadByte();
  item.FileCRC = ReadUInt32();
  item.MTime.DosTime = ReadUInt32();
  item.UnPackVersion = ReadByte();
  item.Method = ReadByte();
  int nameSize = ReadUInt16();
  item.Attrib = ReadUInt32();

  item.MTime.LowSecond = 0;
  item.MTime.SubTime[0] =
      item.MTime.SubTime[1] =
      item.MTime.SubTime[2] = 0;

  if((item.Flags & NHeader::NFile::kSize64Bits) != 0)
  {
    item.PackSize |= ((UInt64)ReadUInt32() << 32);
    item.Size |= ((UInt64)ReadUInt32() << 32);
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
      ReadTime(modifMask, item.MTime);
    item.CTimeDefined = ((createMask & 8) != 0);
    if (item.CTimeDefined)
    {
      item.CTime.DosTime = ReadUInt32();
      ReadTime(createMask, item.CTime);
    }
    item.ATimeDefined = ((accessMask & 8) != 0);
    if (item.ATimeDefined)
    {
      item.ATime.DosTime = ReadUInt32();
      ReadTime(accessMask, item.ATime);
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

HRESULT CInArchive::GetNextItem(CItemEx &item, ICryptoGetTextPassword *getTextPassword, bool &decryptionError, AString &errorMessage)
{
  decryptionError = false;
  for (;;)
  {
    SeekInArchive(m_Position);
    if (!m_CryptoMode && (_header.Flags &
        NHeader::NArchive::kBlockHeadersAreEncrypted) != 0)
    {
      m_CryptoMode = false;
      if (getTextPassword == 0)
        return S_FALSE;
      if (!m_RarAES)
      {
        m_RarAESSpec = new NCrypto::NRar29::CDecoder;
        m_RarAES = m_RarAESSpec;
      }
      m_RarAESSpec->SetRar350Mode(_header.IsEncryptOld());

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
        const UInt32 kAlign = 16;
        m_DecryptedData.SetCapacity(kDecryptedBufferSize + kAlign);
        m_DecryptedDataAligned = (Byte *)((ptrdiff_t)((Byte *)m_DecryptedData + kAlign - 1) & ~(ptrdiff_t)(kAlign - 1));
      }
      RINOK(m_RarAES->Init());
      size_t decryptedDataSizeT = kDecryptedBufferSize;
      RINOK(ReadStream(m_Stream, m_DecryptedDataAligned, &decryptedDataSizeT));
      m_DecryptedDataSize = (UInt32)decryptedDataSizeT;
      m_DecryptedDataSize = m_RarAES->Filter(m_DecryptedDataAligned, m_DecryptedDataSize);

      m_CryptoMode = true;
      m_CryptoPos = 0;
    }

    m_FileHeaderData.EnsureCapacity(7);
    size_t processed = 7;
    RINOK(ReadBytesSpec((Byte *)m_FileHeaderData, &processed));
    if (processed != 7)
    {
      if (processed != 0)
        errorMessage = k_UnexpectedEnd;
      return S_FALSE;
    }

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
      if (!ReadBytesAndTestSize(m_CurData + m_CurPos, m_BlockHeader.HeadSize - 7))
      {
        errorMessage = k_UnexpectedEnd;
        return S_FALSE;
      }

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
    if (m_CryptoMode && m_BlockHeader.HeadSize > (1 << 10))
    {
      decryptionError = true;
      errorMessage = k_DecryptionError;
      return S_FALSE;
    }
    if ((m_BlockHeader.Flags & NHeader::NBlock::kLongBlock) != 0)
    {
      m_FileHeaderData.EnsureCapacity(7 + 4);
      m_CurData = (Byte *)m_FileHeaderData;
      if (!ReadBytesAndTestSize(m_CurData + m_CurPos, 4))
      {
        errorMessage = k_UnexpectedEnd;
        return S_FALSE;
      }
      m_PosLimit = 7 + 4;
      UInt32 dataSize = ReadUInt32();
      AddToSeekValue(dataSize);
      if (m_CryptoMode && dataSize > (1 << 27))
      {
        decryptionError = true;
        errorMessage = k_DecryptionError;
        return S_FALSE;
      }
      m_CryptoPos = m_BlockHeader.HeadSize;
    }
    else
      m_CryptoPos = 0;
    AddToSeekValue(m_BlockHeader.HeadSize);
    FinishCryptoBlock();
    m_CryptoMode = false;
  }
}

void CInArchive::SeekInArchive(UInt64 position)
{
  m_Stream->Seek(position, STREAM_SEEK_SET, NULL);
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
