// RarHandler.cpp

#include "StdAfx.h"

#include "../../../../C/CpuArch.h"

#include "../../../Common/ComTry.h"
#include "../../../Common/IntToString.h"
#include "../../../Common/UTFConvert.h"

#include "../../../Windows/PropVariantUtils.h"
#include "../../../Windows/TimeUtils.h"

#include "../../IPassword.h"

#include "../../Common/CreateCoder.h"
#include "../../Common/FilterCoder.h"
#include "../../Common/LimitedStreams.h"
#include "../../Common/MethodId.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/RegisterArc.h"
#include "../../Common/StreamUtils.h"

#include "../../Compress/CopyCoder.h"

#include "../../Crypto/Rar20Crypto.h"
#include "../../Crypto/RarAes.h"

#include "../Common/FindSignature.h"
#include "../Common/ItemNameUtils.h"
#include "../Common/OutStreamWithCRC.h"

#include "RarHandler.h"

using namespace NWindows;

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)

namespace NArchive {
namespace NRar {

#define SIGNATURE { 0x52 , 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00 }

static const Byte kMarker[NHeader::kMarkerSize] = SIGNATURE;

bool CItem::IgnoreItem() const
{
  switch (HostOS)
  {
    case NHeader::NFile::kHostMSDOS:
    case NHeader::NFile::kHostOS2:
    case NHeader::NFile::kHostWin32:
      return ((Attrib & NHeader::NFile::kLabelFileAttribute) != 0);
  }
  return false;
}

bool CItem::IsDir() const
{
  if (GetDictSize() == NHeader::NFile::kDictDirectoryValue)
    return true;
  switch (HostOS)
  {
    case NHeader::NFile::kHostMSDOS:
    case NHeader::NFile::kHostOS2:
    case NHeader::NFile::kHostWin32:
      if ((Attrib & FILE_ATTRIBUTE_DIRECTORY) != 0)
        return true;
  }
  return false;
}

UInt32 CItem::GetWinAttrib() const
{
  UInt32 a;
  switch (HostOS)
  {
    case NHeader::NFile::kHostMSDOS:
    case NHeader::NFile::kHostOS2:
    case NHeader::NFile::kHostWin32:
      a = Attrib;
      break;
    default:
      a = 0; // must be converted from unix value;
  }
  if (IsDir())
    a |= NHeader::NFile::kWinFileDirectoryAttributeMask;
  return a;
}
  
static const char * const kHostOS[] =
{
    "MS DOS"
  , "OS/2"
  , "Win32"
  , "Unix"
  , "Mac OS"
  , "BeOS"
};

static const char *kUnknownOS = "Unknown";

static const CUInt32PCharPair k_Flags[] =
{
  { 0, "Volume" },
  { 1, "Comment" },
  { 2, "Lock" },
  { 3, "Solid" },
  { 4, "NewVolName" }, // pack_comment in old versuons
  { 5, "Authenticity" },
  { 6, "Recovery" },
  { 7, "BlockEncryption" },
  { 8, "FirstVolume" },
  { 9, "EncryptVer" }
};

enum EErrorType
{
  k_ErrorType_OK,
  k_ErrorType_Corrupted,
  k_ErrorType_UnexpectedEnd,
  k_ErrorType_DecryptionError
};

class CInArchive
{
  IInStream *m_Stream;
  UInt64 m_StreamStartPosition;
  CBuffer<wchar_t> _unicodeNameBuffer;
  CByteBuffer _comment;
  CByteBuffer m_FileHeaderData;
  NHeader::NBlock::CBlock m_BlockHeader;
  NCrypto::NRar29::CDecoder *m_RarAESSpec;
  CMyComPtr<ICompressFilter> m_RarAES;
  CBuffer<Byte> m_DecryptedData;
  Byte *m_DecryptedDataAligned;
  UInt32 m_DecryptedDataSize;
  bool m_CryptoMode;
  UInt32 m_CryptoPos;


  HRESULT ReadBytesSpec(void *data, size_t *size);
  bool ReadBytesAndTestSize(void *data, UInt32 size);
  void ReadName(const Byte *p, unsigned nameSize, CItem &item);
  bool ReadHeaderReal(const Byte *p, unsigned size, CItem &item);
  
  HRESULT Open2(IInStream *stream, const UInt64 *searchHeaderSizeLimit);

  void AddToSeekValue(UInt64 addValue)
  {
    m_Position += addValue;
  }

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
  CInArcInfo ArcInfo;
  bool HeaderErrorWarning;

  HRESULT Open(IInStream *inStream, const UInt64 *searchHeaderSizeLimit);
  HRESULT GetNextItem(CItem &item, ICryptoGetTextPassword *getTextPassword,
      bool &filled, EErrorType &error);
};
  
static bool CheckHeaderCrc(const Byte *header, size_t headerSize)
{
  return Get16(header) == (UInt16)(CrcCalc(header + 2, headerSize - 2) & 0xFFFF);
}

HRESULT CInArchive::Open(IInStream *stream, const UInt64 *searchHeaderSizeLimit)
{
  HeaderErrorWarning = false;
  m_CryptoMode = false;
  RINOK(stream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition));
  RINOK(stream->Seek(0, STREAM_SEEK_END, &ArcInfo.FileSize));
  RINOK(stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL));
  m_Position = m_StreamStartPosition;

  UInt64 arcStartPos = m_StreamStartPosition;
  {
    Byte marker[NHeader::kMarkerSize];
    RINOK(ReadStream_FALSE(stream, marker, NHeader::kMarkerSize));
    if (memcmp(marker, kMarker, NHeader::kMarkerSize) == 0)
      m_Position += NHeader::kMarkerSize;
    else
    {
      if (searchHeaderSizeLimit && *searchHeaderSizeLimit == 0)
        return S_FALSE;
      RINOK(stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL));
      RINOK(FindSignatureInStream(stream, kMarker, NHeader::kMarkerSize,
          searchHeaderSizeLimit, arcStartPos));
      m_Position = arcStartPos + NHeader::kMarkerSize;
      RINOK(stream->Seek(m_Position, STREAM_SEEK_SET, NULL));
    }
  }
  Byte buf[NHeader::NArchive::kArchiveHeaderSize + 1];

  RINOK(ReadStream_FALSE(stream, buf, NHeader::NArchive::kArchiveHeaderSize));
  AddToSeekValue(NHeader::NArchive::kArchiveHeaderSize);


  UInt32 blockSize = Get16(buf + 5);

  ArcInfo.EncryptVersion = 0;
  ArcInfo.Flags = Get16(buf + 3);

  UInt32 headerSize = NHeader::NArchive::kArchiveHeaderSize;
  if (ArcInfo.IsThereEncryptVer())
  {
    if (blockSize <= headerSize)
      return S_FALSE;
    RINOK(ReadStream_FALSE(stream, buf + NHeader::NArchive::kArchiveHeaderSize, 1));
    AddToSeekValue(1);
    ArcInfo.EncryptVersion = buf[NHeader::NArchive::kArchiveHeaderSize];
    headerSize += 1;
  }
  if (blockSize < headerSize
      || buf[2] != NHeader::NBlockType::kArchiveHeader
      || !CheckHeaderCrc(buf, headerSize))
    return S_FALSE;

  size_t commentSize = blockSize - headerSize;
  _comment.Alloc(commentSize);
  RINOK(ReadStream_FALSE(stream, _comment, commentSize));
  AddToSeekValue(commentSize);
  m_Stream = stream;
  ArcInfo.StartPos = arcStartPos;
  return S_OK;
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

static void DecodeUnicodeFileName(const Byte *name, const Byte *encName,
    unsigned encSize, wchar_t *unicodeName, unsigned maxDecSize)
{
  unsigned encPos = 0;
  unsigned decPos = 0;
  unsigned flagBits = 0;
  Byte flags = 0;
  Byte highByte = encName[encPos++];
  while (encPos < encSize && decPos < maxDecSize)
  {
    if (flagBits == 0)
    {
      flags = encName[encPos++];
      flagBits = 8;
    }
    switch (flags >> 6)
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
          unsigned len = encName[encPos++];
          if (len & 0x80)
          {
            Byte correction = encName[encPos++];
            for (len = (len & 0x7f) + 2;
                len > 0 && decPos < maxDecSize; len--, decPos++)
              unicodeName[decPos] = (wchar_t)(((name[decPos] + correction) & 0xff) + (highByte << 8));
          }
          else
            for (len += 2; len > 0 && decPos < maxDecSize; len--, decPos++)
              unicodeName[decPos] = name[decPos];
        }
        break;
    }
    flags <<= 2;
    flagBits -= 2;
  }
  unicodeName[decPos < maxDecSize ? decPos : maxDecSize - 1] = 0;
}

void CInArchive::ReadName(const Byte *p, unsigned nameSize, CItem &item)
{
  item.UnicodeName.Empty();
  if (nameSize > 0)
  {
    unsigned i;
    for (i = 0; i < nameSize && p[i] != 0; i++);
    item.Name.SetFrom((const char *)p, i);

    if (item.HasUnicodeName())
    {
      if (i < nameSize)
      {
        i++;
        unsigned uNameSizeMax = MyMin(nameSize, (unsigned)0x400);
        _unicodeNameBuffer.AllocAtLeast(uNameSizeMax + 1);
        DecodeUnicodeFileName(p, p + i, nameSize - i, _unicodeNameBuffer, uNameSizeMax);
        item.UnicodeName = _unicodeNameBuffer;
      }
      else if (!ConvertUTF8ToUnicode(item.Name, item.UnicodeName))
        item.UnicodeName.Empty();
    }
  }
  else
    item.Name.Empty();
}

static int ReadTime(const Byte *p, unsigned size, Byte mask, CRarTime &rarTime)
{
  rarTime.LowSecond = (Byte)(((mask & 4) != 0) ? 1 : 0);
  unsigned numDigits = (mask & 3);
  rarTime.SubTime[0] =
  rarTime.SubTime[1] =
  rarTime.SubTime[2] = 0;
  if (numDigits > size)
    return -1;
  for (unsigned i = 0; i < numDigits; i++)
    rarTime.SubTime[3 - numDigits + i] = p[i];
  return numDigits;
}

#define READ_TIME(_mask_, _ttt_) \
  { int size2 = ReadTime(p, size, _mask_, _ttt_); if (size2 < 0) return false; p += (unsigned)size2, size -= (unsigned)size2; }

#define READ_TIME_2(_mask_, _def_, _ttt_) \
    _def_ = ((_mask_ & 8) != 0); if (_def_) \
    { if (size < 4) return false; \
      _ttt_ .DosTime = Get32(p); p += 4; size -= 4; \
      READ_TIME(_mask_, _ttt_); } \

bool CInArchive::ReadHeaderReal(const Byte *p, unsigned size, CItem &item)
{
  const Byte *pStart = p;

  item.Clear();
  item.Flags = m_BlockHeader.Flags;

  const unsigned kFileHeaderSize = 25;

  if (size < kFileHeaderSize)
    return false;

  item.PackSize = Get32(p);
  item.Size = Get32(p + 4);
  item.HostOS = p[8];
  item.FileCRC = Get32(p + 9);
  item.MTime.DosTime = Get32(p + 13);
  item.UnPackVersion = p[17];
  item.Method = p[18];
  unsigned nameSize = Get16(p + 19);
  item.Attrib = Get32(p + 21);

  item.MTime.LowSecond = 0;
  item.MTime.SubTime[0] =
      item.MTime.SubTime[1] =
      item.MTime.SubTime[2] = 0;

  p += kFileHeaderSize;
  size -= kFileHeaderSize;
  if ((item.Flags & NHeader::NFile::kSize64Bits) != 0)
  {
    if (size < 8)
      return false;
    item.PackSize |= ((UInt64)Get32(p) << 32);
    item.Size |= ((UInt64)Get32(p + 4) << 32);
    p += 8;
    size -= 8;
  }
  if (nameSize > size)
    return false;
  ReadName(p, nameSize, item);
  p += nameSize;
  size -= nameSize;

  /*
  // It was commented, since it's difficult to support alt Streams for solid archives.
  if (m_BlockHeader.Type == NHeader::NBlockType::kSubBlock)
  {
    if (item.HasSalt())
    {
      if (size < sizeof(item.Salt))
        return false;
      size -= sizeof(item.Salt);
      p += sizeof(item.Salt);
    }
    if (item.Name == "ACL" && size == 0)
    {
      item.IsAltStream = true;
      item.Name.Empty();
      item.UnicodeName.SetFromAscii(".ACL");
    }
    else if (item.Name == "STM" && size != 0 && (size & 1) == 0)
    {
      item.IsAltStream = true;
      item.Name.Empty();
      for (UInt32 i = 0; i < size; i += 2)
      {
        wchar_t c = Get16(p + i);
        if (c == 0)
          return false;
        item.UnicodeName += c;
      }
    }
  }
  */

  if (item.HasSalt())
  {
    if (size < sizeof(item.Salt))
      return false;
    for (unsigned i = 0; i < sizeof(item.Salt); i++)
      item.Salt[i] = p[i];
    p += sizeof(item.Salt);
    size -= sizeof(item.Salt);
  }

  // some rar archives have HasExtTime flag without field.
  if (size >= 2 && item.HasExtTime())
  {
    Byte aMask = (Byte)(p[0] >> 4);
    Byte b = p[1];
    p += 2;
    size -= 2;
    Byte mMask = (Byte)(b >> 4);
    Byte cMask = (Byte)(b & 0xF);
    if ((mMask & 8) != 0)
    {
      READ_TIME(mMask, item.MTime);
    }
    READ_TIME_2(cMask, item.CTimeDefined, item.CTime);
    READ_TIME_2(aMask, item.ATimeDefined, item.ATime);
  }

  unsigned fileHeaderWithNameSize = 7 + (unsigned)(p - pStart);
  
  item.Position = m_Position;
  item.MainPartSize = fileHeaderWithNameSize;
  item.CommentSize = (UInt16)(m_BlockHeader.HeadSize - fileHeaderWithNameSize);

  if (m_CryptoMode)
    item.AlignSize = (UInt16)((16 - ((m_BlockHeader.HeadSize) & 0xF)) & 0xF);
  else
    item.AlignSize = 0;
  AddToSeekValue(m_BlockHeader.HeadSize);
  
  // return (m_BlockHeader.Type != NHeader::NBlockType::kSubBlock || item.IsAltStream);
  return true;
}

HRESULT CInArchive::GetNextItem(CItem &item, ICryptoGetTextPassword *getTextPassword, bool &filled, EErrorType &error)
{
  filled = false;
  error = k_ErrorType_OK;
  for (;;)
  {
    m_Stream->Seek(m_Position, STREAM_SEEK_SET, NULL);
    ArcInfo.EndPos = m_Position;
    if (!m_CryptoMode && (ArcInfo.Flags &
        NHeader::NArchive::kBlockHeadersAreEncrypted) != 0)
    {
      m_CryptoMode = false;
      if (getTextPassword == 0)
      {
        error = k_ErrorType_DecryptionError;
        return S_OK; // return S_FALSE;
      }
      if (!m_RarAES)
      {
        m_RarAESSpec = new NCrypto::NRar29::CDecoder;
        m_RarAES = m_RarAESSpec;
      }
      m_RarAESSpec->SetRar350Mode(ArcInfo.IsEncryptOld());

      // Salt
      const UInt32 kSaltSize = 8;
      Byte salt[kSaltSize];
      if (!ReadBytesAndTestSize(salt, kSaltSize))
        return S_FALSE;
      m_Position += kSaltSize;
      RINOK(m_RarAESSpec->SetDecoderProperties2(salt, kSaltSize))
      // Password
      CMyComBSTR password;
      RINOK(getTextPassword->CryptoGetTextPassword(&password))
      unsigned len = 0;
      if (password)
        len = MyStringLen(password);
      CByteBuffer buffer(len * 2);
      for (unsigned i = 0; i < len; i++)
      {
        wchar_t c = password[i];
        ((Byte *)buffer)[i * 2] = (Byte)c;
        ((Byte *)buffer)[i * 2 + 1] = (Byte)(c >> 8);
      }

      RINOK(m_RarAESSpec->CryptoSetPassword((const Byte *)buffer, (UInt32)buffer.Size()));

      const UInt32 kDecryptedBufferSize = (1 << 12);
      if (m_DecryptedData.Size() == 0)
      {
        const UInt32 kAlign = 16;
        m_DecryptedData.Alloc(kDecryptedBufferSize + kAlign);
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

    m_FileHeaderData.AllocAtLeast(7);
    size_t processed = 7;
    RINOK(ReadBytesSpec((Byte *)m_FileHeaderData, &processed));
    if (processed != 7)
    {
      if (processed != 0)
        error = k_ErrorType_UnexpectedEnd;
      ArcInfo.EndPos = m_Position + processed; // test it
      return S_OK;
    }

    const Byte *p = m_FileHeaderData;
    m_BlockHeader.CRC = Get16(p + 0);
    m_BlockHeader.Type = p[2];
    m_BlockHeader.Flags = Get16(p + 3);
    m_BlockHeader.HeadSize = Get16(p + 5);

    if (m_BlockHeader.HeadSize < 7)
    {
      error = k_ErrorType_Corrupted;
      return S_OK;
      // ThrowExceptionWithCode(CInArchiveException::kIncorrectArchive);
    }

    if (m_BlockHeader.Type < NHeader::NBlockType::kFileHeader ||
        m_BlockHeader.Type > NHeader::NBlockType::kEndOfArchive)
    {
      error = m_CryptoMode ?
          k_ErrorType_DecryptionError :
          k_ErrorType_Corrupted;
      return S_OK;
    }

    if (m_BlockHeader.Type == NHeader::NBlockType::kEndOfArchive)
    {
      bool footerError = false;

      unsigned expectHeadLen = 7;
      if (m_BlockHeader.Flags & NHeader::NArchive::kEndOfArc_Flags_DataCRC)
        expectHeadLen += 4;
      if (m_BlockHeader.Flags & NHeader::NArchive::kEndOfArc_Flags_VolNumber)
        expectHeadLen += 2;
      if (m_BlockHeader.Flags & NHeader::NArchive::kEndOfArc_Flags_RevSpace)
        expectHeadLen += 7;

      // rar 5.0 beta 1 writes incorrect RevSpace and headSize

      if (m_BlockHeader.HeadSize < expectHeadLen)
        HeaderErrorWarning = true;
        
      if (m_BlockHeader.HeadSize > 7)
      {
        /* We suppose that EndOfArchive header is always small.
           It's only 20 bytes for multivolume
           Fix the limit, if larger footers are possible */
        if (m_BlockHeader.HeadSize > (1 << 8))
          footerError = true;
        else
        {
          if (m_FileHeaderData.Size() < m_BlockHeader.HeadSize)
            m_FileHeaderData.ChangeSize_KeepData(m_BlockHeader.HeadSize, 7);
          UInt32 afterSize = m_BlockHeader.HeadSize - 7;
          if (ReadBytesAndTestSize(m_FileHeaderData + 7, afterSize))
            processed += afterSize;
          else
          {
            if (!m_CryptoMode)
            {
              error = k_ErrorType_UnexpectedEnd;
              return S_OK;
            }
            footerError = true;
          }
        }
      }
      
      if (footerError || !CheckHeaderCrc(m_FileHeaderData, m_BlockHeader.HeadSize))
      {
        error = m_CryptoMode ?
          k_ErrorType_DecryptionError :
          k_ErrorType_Corrupted;
      }
      else
      {
        ArcInfo.EndFlags = m_BlockHeader.Flags;
        UInt32 offset = 7;
        if (m_BlockHeader.Flags & NHeader::NArchive::kEndOfArc_Flags_DataCRC)
        {
          if (processed < offset + 4)
            error = k_ErrorType_Corrupted;
          else
            ArcInfo.DataCRC = Get32(m_FileHeaderData + offset);
          offset += 4;
        }
        if (m_BlockHeader.Flags & NHeader::NArchive::kEndOfArc_Flags_VolNumber)
        {
          if (processed < offset + 2)
            error = k_ErrorType_Corrupted;
          ArcInfo.VolNumber = (UInt32)Get16(m_FileHeaderData + offset);
        }

        ArcInfo.EndOfArchive_was_Read = true;
      }
      m_Position += processed;
      FinishCryptoBlock();
      ArcInfo.EndPos = m_Position;
      return S_OK;
    }

    if (m_BlockHeader.Type == NHeader::NBlockType::kFileHeader
        /* || m_BlockHeader.Type == NHeader::NBlockType::kSubBlock */)
    {
      if (m_FileHeaderData.Size() < m_BlockHeader.HeadSize)
        m_FileHeaderData.ChangeSize_KeepData(m_BlockHeader.HeadSize, 7);
      // m_CurData = (Byte *)m_FileHeaderData;
      // m_PosLimit = m_BlockHeader.HeadSize;
      if (!ReadBytesAndTestSize(m_FileHeaderData + 7, m_BlockHeader.HeadSize - 7))
      {
        error = k_ErrorType_UnexpectedEnd;
        return S_OK;
      }

      bool okItem = ReadHeaderReal(m_FileHeaderData + 7, m_BlockHeader.HeadSize - 7, item);
      if (okItem)
      {
        if (!CheckHeaderCrc(m_FileHeaderData, (unsigned)m_BlockHeader.HeadSize - item.CommentSize))
        {
          error = k_ErrorType_Corrupted; // ThrowExceptionWithCode(CInArchiveException::kFileHeaderCRCError);
          return S_OK;
        }
        filled = true;
      }

      FinishCryptoBlock();
      m_CryptoMode = false;
      // Move Position to compressed Data;
      m_Stream->Seek(m_Position, STREAM_SEEK_SET, NULL);
      AddToSeekValue(item.PackSize);  // m_Position points to next header;
      // if (okItem)
        return S_OK;
      /*
      else
        continue;
      */
    }
    if (m_CryptoMode && m_BlockHeader.HeadSize > (1 << 10))
    {
      error = k_ErrorType_DecryptionError;
      return S_OK;
    }
    if ((m_BlockHeader.Flags & NHeader::NBlock::kLongBlock) != 0)
    {
      if (m_FileHeaderData.Size() < 7 + 4)
        m_FileHeaderData.ChangeSize_KeepData(7 + 4, 7);
      if (!ReadBytesAndTestSize(m_FileHeaderData + 7, 4))
      {
        error = k_ErrorType_UnexpectedEnd;
        return S_OK;
      }
      UInt32 dataSize = Get32(m_FileHeaderData + 7);
      AddToSeekValue(dataSize);
      if (m_CryptoMode && dataSize > (1 << 27))
      {
        error = k_ErrorType_DecryptionError;
        return S_OK;
      }
      m_CryptoPos = m_BlockHeader.HeadSize;
    }
    else
      m_CryptoPos = 0;
    
    {
      UInt64 newPos = m_Position + m_BlockHeader.HeadSize;
      if (newPos > ArcInfo.FileSize)
      {
        error = k_ErrorType_UnexpectedEnd;
        return S_OK;
      }
    }
    AddToSeekValue(m_BlockHeader.HeadSize);
    FinishCryptoBlock();
    m_CryptoMode = false;
  }
}


static const Byte kProps[] =
{
  kpidPath,
  kpidIsDir,
  kpidSize,
  kpidPackSize,
  kpidMTime,
  kpidCTime,
  kpidATime,
  kpidAttrib,

  kpidEncrypted,
  kpidSolid,
  kpidCommented,
  kpidSplitBefore,
  kpidSplitAfter,
  kpidCRC,
  kpidHostOS,
  kpidMethod,
  kpidUnpackVer
};

static const Byte kArcProps[] =
{
  kpidTotalPhySize,
  kpidCharacts,
  kpidSolid,
  kpidNumBlocks,
  // kpidEncrypted,
  kpidIsVolume,
  kpidVolumeIndex,
  kpidNumVolumes
  // kpidCommented
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

UInt64 CHandler::GetPackSize(unsigned refIndex) const
{
  const CRefItem &refItem = _refItems[refIndex];
  UInt64 totalPackSize = 0;
  for (unsigned i = 0; i < refItem.NumItems; i++)
    totalPackSize += _items[refItem.ItemIndex + i].PackSize;
  return totalPackSize;
}

bool CHandler::IsSolid(unsigned refIndex) const
{
  const CItem &item = _items[_refItems[refIndex].ItemIndex];
  if (item.UnPackVersion < 20)
  {
    if (_arcInfo.IsSolid())
      return (refIndex > 0);
    return false;
  }
  return item.IsSolid();
}

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidVolumeIndex: if (_arcInfo.Is_VolNumber_Defined()) prop = (UInt32)_arcInfo.VolNumber; break;
    case kpidSolid: prop = _arcInfo.IsSolid(); break;
    case kpidCharacts:
    {
      AString s = FlagsToString(k_Flags, ARRAY_SIZE(k_Flags), _arcInfo.Flags);
      // FLAGS_TO_PROP(k_Flags, _arcInfo.Flags, prop);
      if (_arcInfo.Is_DataCRC_Defined())
      {
        s.Add_Space_if_NotEmpty();
        s += "VolCRC";
      }
      prop = s;
      break;
    }
    // case kpidEncrypted: prop = _arcInfo.IsEncrypted(); break; // it's for encrypted names.
    case kpidIsVolume: prop = _arcInfo.IsVolume(); break;
    case kpidNumVolumes: prop = (UInt32)_arcs.Size(); break;
    case kpidOffset: if (_arcs.Size() == 1 && _arcInfo.StartPos != 0) prop = _arcInfo.StartPos; break;

    case kpidTotalPhySize:
    {
      if (_arcs.Size() > 1)
      {
        UInt64 sum = 0;
        FOR_VECTOR (v, _arcs)
          sum += _arcs[v].PhySize;
        prop = sum;
      }
      break;
    }

    case kpidPhySize:
    {
      if (_arcs.Size() != 0)
        prop = _arcInfo.GetPhySize();
      break;
    }

    // case kpidCommented: prop = _arcInfo.IsCommented(); break;

    case kpidNumBlocks:
    {
      UInt32 numBlocks = 0;
      FOR_VECTOR (i, _refItems)
        if (!IsSolid(i))
          numBlocks++;
      prop = (UInt32)numBlocks;
      break;
    }
    
    // case kpidError: if (!_errorMessage.IsEmpty()) prop = _errorMessage; break;
    
    case kpidErrorFlags:
    {
      UInt32 v = _errorFlags;
      if (!_isArc)
        v |= kpv_ErrorFlags_IsNotArc;
      prop = v;
      break;
    }

    case kpidWarningFlags:
    {
      if (_warningFlags != 0)
        prop = _warningFlags;
      break;
    }

    case kpidExtension:
      if (_arcs.Size() == 1)
      {
        if (_arcInfo.Is_VolNumber_Defined())
        {
          char sz[16];
          ConvertUInt32ToString((UInt32)_arcInfo.VolNumber + 1, sz);
          unsigned len = MyStringLen(sz);
          AString s = "part";
          for (; len < 2; len++)
            s += '0';
          s += sz;
          s += ".rar";
          prop = s;
        }
      }
      break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _refItems.Size();
  return S_OK;
}

static bool RarTimeToFileTime(const CRarTime &rarTime, FILETIME &result)
{
  if (!NTime::DosTimeToFileTime(rarTime.DosTime, result))
    return false;
  UInt64 value =  (((UInt64)result.dwHighDateTime) << 32) + result.dwLowDateTime;
  value += (UInt64)rarTime.LowSecond * 10000000;
  value += ((UInt64)rarTime.SubTime[2] << 16) +
    ((UInt64)rarTime.SubTime[1] << 8) +
    ((UInt64)rarTime.SubTime[0]);
  result.dwLowDateTime = (DWORD)value;
  result.dwHighDateTime = DWORD(value >> 32);
  return true;
}

static void RarTimeToProp(const CRarTime &rarTime, NCOM::CPropVariant &prop)
{
  FILETIME localFileTime, utcFileTime;
  if (RarTimeToFileTime(rarTime, localFileTime))
  {
    if (!LocalFileTimeToFileTime(&localFileTime, &utcFileTime))
      utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
  }
  else
    utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
  prop = utcFileTime;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  const CRefItem &refItem = _refItems[index];
  const CItem &item = _items[refItem.ItemIndex];
  /*
  const CItem *mainItem = &item;
  if (item.BaseFileIndex >= 0)
    mainItem = &_items[_refItems[item.BaseFileIndex].ItemIndex];
  */
  switch(propID)
  {
    case kpidPath:
    {
      /*
      UString u;
      if (item.BaseFileIndex >= 0)
        u = mainItem->GetName();
      u += item.GetName();
      */
      prop = (const wchar_t *)NItemName::WinNameToOSName(item.GetName());
      break;
    }
    case kpidIsDir: prop = item.IsDir(); break;
    case kpidSize: prop = item.Size; break;
    case kpidPackSize: prop = GetPackSize(index); break;
    case kpidMTime: RarTimeToProp(item.MTime, prop); break;
    case kpidCTime: if (item.CTimeDefined) RarTimeToProp(item.CTime, prop); break;
    case kpidATime: if (item.ATimeDefined) RarTimeToProp(item.ATime, prop); break;
    case kpidAttrib: prop = item.GetWinAttrib(); break;
    case kpidEncrypted: prop = item.IsEncrypted(); break;
    case kpidSolid: prop = IsSolid(index); break;
    case kpidCommented: prop = item.IsCommented(); break;
    case kpidSplitBefore: prop = item.IsSplitBefore(); break;
    case kpidSplitAfter: prop = _items[refItem.ItemIndex + refItem.NumItems - 1].IsSplitAfter(); break;
    case kpidCRC:
    {
      const CItem &lastItem = _items[refItem.ItemIndex + refItem.NumItems - 1];
      prop = ((lastItem.IsSplitAfter()) ? item.FileCRC : lastItem.FileCRC);
      break;
    }
    case kpidUnpackVer: prop = item.UnPackVersion; break;
    case kpidMethod:
    {
      char s[16];
      Byte m = item.Method;
      if (m < (Byte)'0' || m > (Byte)'5')
        ConvertUInt32ToString(m, s);
      else
      {
        s[0] = 'm';
        s[1] = (char)m;
        s[2] = 0;
        if (!item.IsDir())
        {
          s[2] = ':';
          ConvertUInt32ToString(16 + item.GetDictSize(), &s[3]);
        }
      }
      prop = s;
      break;
    }
    case kpidHostOS: prop = (item.HostOS < ARRAY_SIZE(kHostOS)) ? kHostOS[item.HostOS] : kUnknownOS; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

static bool IsDigit(wchar_t c)
{
  return c >= L'0' && c <= L'9';
}

class CVolumeName
{
  bool _first;
  bool _newStyle;
  UString _unchangedPart;
  UString _changedPart;
  UString _afterPart;
public:
  CVolumeName(): _newStyle(true) {};

  bool InitName(const UString &name, bool newStyle)
  {
    _first = true;
    _newStyle = newStyle;
    int dotPos = name.ReverseFind_Dot();
    UString basePart = name;

    if (dotPos >= 0)
    {
      UString ext = name.Ptr(dotPos + 1);
      if (ext.IsEqualTo_Ascii_NoCase("rar"))
      {
        _afterPart = name.Ptr(dotPos);
        basePart = name.Left(dotPos);
      }
      else if (ext.IsEqualTo_Ascii_NoCase("exe"))
      {
        _afterPart.SetFromAscii(".rar");
        basePart = name.Left(dotPos);
      }
      else if (!_newStyle)
      {
        if (ext.IsEqualTo_Ascii_NoCase("000") ||
            ext.IsEqualTo_Ascii_NoCase("001") ||
            ext.IsEqualTo_Ascii_NoCase("r00") ||
            ext.IsEqualTo_Ascii_NoCase("r01"))
        {
          _afterPart.Empty();
          _first = false;
          _changedPart = ext;
          _unchangedPart = name.Left(dotPos + 1);
          return true;
        }
      }
    }

    if (!_newStyle)
    {
      _afterPart.Empty();
      _unchangedPart = basePart;
      _unchangedPart += L'.';
      _changedPart.SetFromAscii("r00");
      return true;
    }

    if (basePart.IsEmpty())
      return false;
    unsigned i = basePart.Len();
    
    do
      if (!IsDigit(basePart[i - 1]))
        break;
    while (--i);
    
    _unchangedPart = basePart.Left(i);
    _changedPart = basePart.Ptr(i);
    return true;
  }

  /*
  void MakeBeforeFirstName()
  {
    unsigned len = _changedPart.Len();
    _changedPart.Empty();
    for (unsigned i = 0; i < len; i++)
      _changedPart += L'0';
  }
  */

  UString GetNextName()
  {
    if (_newStyle || !_first)
    {
      unsigned i = _changedPart.Len();
      for (;;)
      {
        wchar_t c = _changedPart[--i];
        if (c == L'9')
        {
          c = L'0';
          _changedPart.ReplaceOneCharAtPos(i, c);
          if (i == 0)
          {
            _changedPart.InsertAtFront(L'1');
            break;
          }
          continue;
        }
        c++;
        _changedPart.ReplaceOneCharAtPos(i, c);
        break;
      }
    }
    
    _first = false;
    return _unchangedPart + _changedPart + _afterPart;
  }
};

static HRESULT ReadZeroTail(ISequentialInStream *stream, bool &areThereNonZeros, UInt64 &numZeros, UInt64 maxSize)
{
  areThereNonZeros = false;
  numZeros = 0;
  const size_t kBufSize = 1 << 9;
  Byte buf[kBufSize];
  for (;;)
  {
    UInt32 size = 0;
    HRESULT(stream->Read(buf, kBufSize, &size));
    if (size == 0)
      return S_OK;
    for (UInt32 i = 0; i < size; i++)
      if (buf[i] != 0)
      {
        areThereNonZeros = true;
        numZeros += i;
        return S_OK;
      }
    numZeros += size;
    if (numZeros > maxSize)
      return S_OK;
  }
}

HRESULT CHandler::Open2(IInStream *stream,
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *openCallback)
{
  {
    CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
    CMyComPtr<ICryptoGetTextPassword> getTextPassword;
    CMyComPtr<IArchiveOpenCallback> openArchiveCallbackWrap = openCallback;
    
    CVolumeName seqName;

    UInt64 totalBytes = 0;
    UInt64 curBytes = 0;

    if (openCallback)
    {
      openArchiveCallbackWrap.QueryInterface(IID_IArchiveOpenVolumeCallback, &openVolumeCallback);
      openArchiveCallbackWrap.QueryInterface(IID_ICryptoGetTextPassword, &getTextPassword);
    }

    CInArchive archive;
    for (;;)
    {
      CMyComPtr<IInStream> inStream;
      if (!_arcs.IsEmpty())
      {
        if (!openVolumeCallback)
          break;
        
        if (_arcs.Size() == 1)
        {
          if (!_arcInfo.IsVolume())
            break;
          UString baseName;
          {
            NCOM::CPropVariant prop;
            RINOK(openVolumeCallback->GetProperty(kpidName, &prop));
            if (prop.vt != VT_BSTR)
              break;
            baseName = prop.bstrVal;
          }
          if (!seqName.InitName(baseName, _arcInfo.HaveNewVolumeName()))
            break;
          /*
          if (_arcInfo.HaveNewVolumeName() && !_arcInfo.IsFirstVolume())
          {
            seqName.MakeBeforeFirstName();
          }
          */
        }

        UString fullName = seqName.GetNextName();
        HRESULT result = openVolumeCallback->GetStream(fullName, &inStream);
        if (result == S_FALSE)
          break;
        if (result != S_OK)
          return result;
        if (!inStream)
          break;
      }
      else
        inStream = stream;

      UInt64 endPos = 0;
      RINOK(inStream->Seek(0, STREAM_SEEK_END, &endPos));
      RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
      if (openCallback)
      {
        totalBytes += endPos;
        RINOK(openCallback->SetTotal(NULL, &totalBytes));
      }
      
      RINOK(archive.Open(inStream, maxCheckStartPosition));
      _isArc = true;
      CItem item;
      
      for (;;)
      {
        if (archive.m_Position > endPos)
        {
          _errorFlags |= kpv_ErrorFlags_UnexpectedEnd;
          break;
        }
        
        EErrorType error;
        // bool decryptionError;
        // AString errorMessageLoc;
        bool filled;
        HRESULT result = archive.GetNextItem(item, getTextPassword, filled, error);
        
        if (error != k_ErrorType_OK)
        {
          if (error == k_ErrorType_UnexpectedEnd)
            _errorFlags |= kpv_ErrorFlags_UnexpectedEnd;
          else if (error == k_ErrorType_Corrupted)
            _errorFlags |= kpv_ErrorFlags_HeadersError;
          else if (error == k_ErrorType_DecryptionError)
            _errorFlags |= kpv_ErrorFlags_EncryptedHeadersError;

          // AddErrorMessage(errorMessageLoc);
        }
        RINOK(result);
        
        if (!filled)
        {
          if (error == k_ErrorType_DecryptionError && _items.IsEmpty())
            return S_FALSE;

          if (archive.ArcInfo.ExtraZeroTail_is_Possible())
          {
            /* if there is recovery record for multivolume archive,
               RAR adds 18 bytes (ZERO bytes) at the end for alignment.
               We must skip these bytes to prevent phySize warning. */
            RINOK(inStream->Seek(archive.ArcInfo.EndPos, STREAM_SEEK_SET, NULL));
            bool areThereNonZeros;
            UInt64 numZeros;
            const UInt64 maxSize = 1 << 12;
            RINOK(ReadZeroTail(inStream, areThereNonZeros, numZeros, maxSize));
            if (!areThereNonZeros && numZeros != 0 && numZeros <= maxSize)
              archive.ArcInfo.EndPos += numZeros;
          }
          break;
        }
        
        if (item.IgnoreItem())
          continue;

        bool needAdd = true;
        
        if (item.IsSplitBefore())
        {
          if (!_refItems.IsEmpty())
          {
            CRefItem &refItem = _refItems.Back();
            refItem.NumItems++;
            needAdd = false;
          }
        }
        
        if (needAdd)
        {
          CRefItem refItem;
          refItem.ItemIndex = _items.Size();
          refItem.NumItems = 1;
          refItem.VolumeIndex = _arcs.Size();
          _refItems.Add(refItem);
        }

        _items.Add(item);
        
        if (openCallback && _items.Size() % 100 == 0)
        {
          UInt64 numFiles = _items.Size();
          UInt64 numBytes = curBytes + item.Position;
          RINOK(openCallback->SetCompleted(&numFiles, &numBytes));
        }
      }

      if (archive.HeaderErrorWarning)
        _warningFlags |= kpv_ErrorFlags_HeadersError;

      /*
      if (archive.m_Position < endPos)
        _warningFlags |= kpv_ErrorFlags_DataAfterEnd;
      */
      if (_arcs.IsEmpty())
        _arcInfo = archive.ArcInfo;
      // _arcInfo.EndPos = archive.EndPos;

      curBytes += endPos;
      {
        CArc &arc = _arcs.AddNew();
        arc.PhySize = archive.ArcInfo.GetPhySize();
        arc.Stream = inStream;
      }
    }
  }

  /*
  int baseFileIndex = -1;
  for (unsigned i = 0; i < _refItems.Size(); i++)
  {
    CItem &item = _items[_refItems[i].ItemIndex];
    if (item.IsAltStream)
      item.BaseFileIndex = baseFileIndex;
    else
      baseFileIndex = i;
  }
  */
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *openCallback)
{
  COM_TRY_BEGIN
  Close();
  // try
  {
    HRESULT res = Open2(stream, maxCheckStartPosition, openCallback);
    /*
    if (res != S_OK)
      Close();
    */

    return res;
  }
  // catch(const CInArchiveException &) { Close(); return S_FALSE; }
  // catch(...) { Close(); throw; }
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  COM_TRY_BEGIN
  // _errorMessage.Empty();
  _errorFlags = 0;
  _warningFlags = 0;
  _isArc = false;
  _refItems.Clear();
  _items.Clear();
  _arcs.Clear();
  return S_OK;
  COM_TRY_END
}

struct CMethodItem
{
  Byte RarUnPackVersion;
  CMyComPtr<ICompressCoder> Coder;
};


class CFolderInStream:
  public ISequentialInStream,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);

private:
  const CObjectVector<CArc> *_archives;
  const CObjectVector<CItem> *_items;
  CRefItem _refItem;
  unsigned _curIndex;
  UInt32 _crc;
  bool _fileIsOpen;
  CMyComPtr<ISequentialInStream> _stream;

  HRESULT OpenStream();
  HRESULT CloseStream();
public:
  void Init(const CObjectVector<CArc> *archives,
      const CObjectVector<CItem> *items,
      const CRefItem &refItem);

  CRecordVector<UInt32> CRCs;
};


ISequentialInStream* CArc::CreateLimitedStream(UInt64 offset, UInt64 size) const
{
  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  Stream->Seek(offset, STREAM_SEEK_SET, NULL);
  streamSpec->SetStream(Stream);
  streamSpec->Init(size);
  return inStream.Detach();
}

void CFolderInStream::Init(
    const CObjectVector<CArc> *archives,
    const CObjectVector<CItem> *items,
    const CRefItem &refItem)
{
  _archives = archives;
  _items = items;
  _refItem = refItem;
  _curIndex = 0;
  CRCs.Clear();
  _fileIsOpen = false;
}

HRESULT CFolderInStream::OpenStream()
{
  while (_curIndex < _refItem.NumItems)
  {
    const CItem &item = (*_items)[_refItem.ItemIndex + _curIndex];
    _stream.Attach((*_archives)[_refItem.VolumeIndex + _curIndex].
        CreateLimitedStream(item.GetDataPosition(), item.PackSize));
    _curIndex++;
    _fileIsOpen = true;
    _crc = CRC_INIT_VAL;
    return S_OK;
  }
  return S_OK;
}

HRESULT CFolderInStream::CloseStream()
{
  CRCs.Add(CRC_GET_DIGEST(_crc));
  _stream.Release();
  _fileIsOpen = false;
  return S_OK;
}

STDMETHODIMP CFolderInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize = 0;
  while ((_curIndex < _refItem.NumItems || _fileIsOpen) && size > 0)
  {
    if (_fileIsOpen)
    {
      UInt32 localProcessedSize;
      RINOK(_stream->Read(
          ((Byte *)data) + realProcessedSize, size, &localProcessedSize));
      _crc = CrcUpdate(_crc, ((Byte *)data) + realProcessedSize, localProcessedSize);
      if (localProcessedSize == 0)
      {
        RINOK(CloseStream());
        continue;
      }
      realProcessedSize += localProcessedSize;
      size -= localProcessedSize;
      break;
    }
    else
    {
      RINOK(OpenStream());
    }
  }
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  CMyComPtr<ICryptoGetTextPassword> getTextPassword;
  UInt64 censoredTotalUnPacked = 0,
        // censoredTotalPacked = 0,
        importantTotalUnPacked = 0;
        // importantTotalPacked = 0;
  bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = _refItems.Size();
  if (numItems == 0)
    return S_OK;
  unsigned lastIndex = 0;
  CRecordVector<unsigned> importantIndexes;
  CRecordVector<bool> extractStatuses;

  for (UInt32 t = 0; t < numItems; t++)
  {
    unsigned index = allFilesMode ? t : indices[t];
    const CRefItem &refItem = _refItems[index];
    const CItem &item = _items[refItem.ItemIndex];
    censoredTotalUnPacked += item.Size;
    // censoredTotalPacked += item.PackSize;
    unsigned j;
    for (j = lastIndex; j <= index; j++)
      // if (!_items[_refItems[j].ItemIndex].IsSolid())
      if (!IsSolid(j))
        lastIndex = j;
    for (j = lastIndex; j <= index; j++)
    {
      const CRefItem &refItem = _refItems[j];
      const CItem &item = _items[refItem.ItemIndex];

      // const CItem &item = _items[j];

      importantTotalUnPacked += item.Size;
      // importantTotalPacked += item.PackSize;
      importantIndexes.Add(j);
      extractStatuses.Add(j == index);
    }
    lastIndex = index + 1;
  }

  RINOK(extractCallback->SetTotal(importantTotalUnPacked));
  UInt64 currentImportantTotalUnPacked = 0;
  UInt64 currentImportantTotalPacked = 0;
  UInt64 currentUnPackSize, currentPackSize;

  CObjectVector<CMethodItem> methodItems;

  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder;
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CFilterCoder *filterStreamSpec = new CFilterCoder(false);
  CMyComPtr<ISequentialInStream> filterStream = filterStreamSpec;

  NCrypto::NRar20::CDecoder *rar20CryptoDecoderSpec = NULL;
  CMyComPtr<ICompressFilter> rar20CryptoDecoder;
  NCrypto::NRar29::CDecoder *rar29CryptoDecoderSpec = NULL;
  CMyComPtr<ICompressFilter> rar29CryptoDecoder;

  CFolderInStream *folderInStreamSpec = NULL;
  CMyComPtr<ISequentialInStream> folderInStream;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  bool solidStart = true;
  for (unsigned i = 0; i < importantIndexes.Size(); i++,
      currentImportantTotalUnPacked += currentUnPackSize,
      currentImportantTotalPacked += currentPackSize)
  {
    lps->InSize = currentImportantTotalPacked;
    lps->OutSize = currentImportantTotalUnPacked;
    RINOK(lps->SetCur());
    CMyComPtr<ISequentialOutStream> realOutStream;

    Int32 askMode;
    if (extractStatuses[i])
      askMode = testMode ?
          NExtract::NAskMode::kTest :
          NExtract::NAskMode::kExtract;
    else
      askMode = NExtract::NAskMode::kSkip;

    UInt32 index = importantIndexes[i];

    const CRefItem &refItem = _refItems[index];
    const CItem &item = _items[refItem.ItemIndex];

    currentUnPackSize = item.Size;

    currentPackSize = GetPackSize(index);

    if (item.IgnoreItem())
      continue;

    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    if (!IsSolid(index))
      solidStart = true;
    if (item.IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      continue;
    }

    bool mustBeProcessedAnywhere = false;
    if (i < importantIndexes.Size() - 1)
    {
      // const CRefItem &nextRefItem = _refItems[importantIndexes[i + 1]];
      // const CItem &nextItemInfo = _items[nextRefItem.ItemIndex];
      // mustBeProcessedAnywhere = nextItemInfo.IsSolid();
      mustBeProcessedAnywhere = IsSolid(importantIndexes[i + 1]);
    }
    
    if (!mustBeProcessedAnywhere && !testMode && !realOutStream)
      continue;
    
    if (!realOutStream && !testMode)
      askMode = NExtract::NAskMode::kSkip;

    RINOK(extractCallback->PrepareOperation(askMode));

    COutStreamWithCRC *outStreamSpec = new COutStreamWithCRC;
    CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
    outStreamSpec->SetStream(realOutStream);
    outStreamSpec->Init();
    realOutStream.Release();
    
    /*
    for (unsigned partIndex = 0; partIndex < 1; partIndex++)
    {
    CMyComPtr<ISequentialInStream> inStream;

    // item redefinition
    const CItem &item = _items[refItem.ItemIndex + partIndex];

    CInArchive &archive = _arcs[refItem.VolumeIndex + partIndex];

    inStream.Attach(archive.CreateLimitedStream(item.GetDataPosition(),
      item.PackSize));
    */
    if (!folderInStream)
    {
      folderInStreamSpec = new CFolderInStream;
      folderInStream = folderInStreamSpec;
    }

    folderInStreamSpec->Init(&_arcs, &_items, refItem);

    UInt64 packSize = currentPackSize;

    // packedPos += item.PackSize;
    // unpackedPos += 0;
    
    CMyComPtr<ISequentialInStream> inStream;
    
    if (item.IsEncrypted())
    {
      CMyComPtr<ICryptoSetPassword> cryptoSetPassword;
      
      if (item.UnPackVersion >= 29)
      {
        if (!rar29CryptoDecoder)
        {
          rar29CryptoDecoderSpec = new NCrypto::NRar29::CDecoder;
          rar29CryptoDecoder = rar29CryptoDecoderSpec;
        }
        rar29CryptoDecoderSpec->SetRar350Mode(item.UnPackVersion < 36);
        /*
        CMyComPtr<ICompressSetDecoderProperties2> cryptoProperties;
        RINOK(rar29CryptoDecoder.QueryInterface(IID_ICompressSetDecoderProperties2,
            &cryptoProperties));
        */
        RINOK(rar29CryptoDecoderSpec->SetDecoderProperties2(item.Salt, item.HasSalt() ? sizeof(item.Salt) : 0));
        filterStreamSpec->Filter = rar29CryptoDecoder;
      }
      else if (item.UnPackVersion >= 20)
      {
        if (!rar20CryptoDecoder)
        {
          rar20CryptoDecoderSpec = new NCrypto::NRar20::CDecoder;
          rar20CryptoDecoder = rar20CryptoDecoderSpec;
        }
        filterStreamSpec->Filter = rar20CryptoDecoder;
      }
      else
      {
        outStream.Release();
        RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kUnsupportedMethod));
        continue;
      }
      
      RINOK(filterStreamSpec->Filter.QueryInterface(IID_ICryptoSetPassword, &cryptoSetPassword));

      if (!getTextPassword)
        extractCallback->QueryInterface(IID_ICryptoGetTextPassword, (void **)&getTextPassword);
      if (getTextPassword)
      {
        CMyComBSTR password;
        RINOK(getTextPassword->CryptoGetTextPassword(&password));
        if (item.UnPackVersion >= 29)
        {
          UString unicodePassword;
          unsigned len = 0;
          if (password)
            len = MyStringLen(password);
          CByteBuffer buffer(len * 2);
          for (unsigned i = 0; i < len; i++)
          {
            wchar_t c = password[i];
            ((Byte *)buffer)[i * 2] = (Byte)c;
            ((Byte *)buffer)[i * 2 + 1] = (Byte)(c >> 8);
          }
          RINOK(cryptoSetPassword->CryptoSetPassword((const Byte *)buffer, (UInt32)buffer.Size()));
        }
        else
        {
          AString oemPassword;
          if (password)
            oemPassword = UnicodeStringToMultiByte((const wchar_t *)password, CP_OEMCP);
          RINOK(cryptoSetPassword->CryptoSetPassword((const Byte *)(const char *)oemPassword, oemPassword.Len()));
        }
      }
      else
      {
        RINOK(cryptoSetPassword->CryptoSetPassword(0, 0));
      }
      
      filterStreamSpec->SetInStream(folderInStream);
      filterStreamSpec->SetOutStreamSize(NULL);
      inStream = filterStream;
    }
    else
    {
      inStream = folderInStream;
    }
    
    CMyComPtr<ICompressCoder> commonCoder;
    
    switch(item.Method)
    {
      case '0':
      {
        commonCoder = copyCoder;
        break;
      }
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      {
        unsigned m;
        for (m = 0; m < methodItems.Size(); m++)
          if (methodItems[m].RarUnPackVersion == item.UnPackVersion)
            break;
        if (m == methodItems.Size())
        {
          CMethodItem mi;
          mi.RarUnPackVersion = item.UnPackVersion;

          mi.Coder.Release();
          if (item.UnPackVersion <= 40)
          {
            UInt32 methodID = 0x40300;
            if (item.UnPackVersion < 20)
              methodID += 1;
            else if (item.UnPackVersion < 29)
              methodID += 2;
            else
              methodID += 3;
            RINOK(CreateCoder(EXTERNAL_CODECS_VARS methodID, false, mi.Coder));
          }
         
          if (mi.Coder == 0)
          {
            outStream.Release();
            RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kUnsupportedMethod));
            continue;
          }

          m = methodItems.Add(mi);
        }
        CMyComPtr<ICompressCoder> decoder = methodItems[m].Coder;

        CMyComPtr<ICompressSetDecoderProperties2> compressSetDecoderProperties;
        RINOK(decoder.QueryInterface(IID_ICompressSetDecoderProperties2,
            &compressSetDecoderProperties));
        
        Byte isSolid = (Byte)((IsSolid(index) || item.IsSplitBefore()) ? 1: 0);
        if (solidStart)
        {
          isSolid = false;
          solidStart = false;
        }


        RINOK(compressSetDecoderProperties->SetDecoderProperties2(&isSolid, 1));
          
        commonCoder = decoder;
        break;
      }
      default:
        outStream.Release();
        RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kUnsupportedMethod));
        continue;
    }
    
    HRESULT result = commonCoder->Code(inStream, outStream, &packSize, &item.Size, progress);
    
    if (item.IsEncrypted())
      filterStreamSpec->ReleaseInStream();
    if (result == S_FALSE)
    {
      outStream.Release();
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kDataError));
      continue;
    }
    if (result != S_OK)
      return result;

    /*
    if (refItem.NumItems == 1 &&
        !item.IsSplitBefore() && !item.IsSplitAfter())
    */
    {
      const CItem &lastItem = _items[refItem.ItemIndex + refItem.NumItems - 1];
      bool crcOK = outStreamSpec->GetCRC() == lastItem.FileCRC;
      outStream.Release();
      RINOK(extractCallback->SetOperationResult(crcOK ?
          NExtract::NOperationResult::kOK:
          NExtract::NOperationResult::kCRCError));
    }
    /*
    else
    {
      bool crcOK = true;
      for (unsigned partIndex = 0; partIndex < refItem.NumItems; partIndex++)
      {
        const CItem &item = _items[refItem.ItemIndex + partIndex];
        if (item.FileCRC != folderInStreamSpec->CRCs[partIndex])
        {
          crcOK = false;
          break;
        }
      }
      RINOK(extractCallback->SetOperationResult(crcOK ?
          NExtract::NOperationResult::kOK:
          NExtract::NOperationResult::kCRCError));
    }
    */
  }
  return S_OK;
  COM_TRY_END
}

IMPL_ISetCompressCodecsInfo

REGISTER_ARC_I(
  "Rar", "rar r00", 0, 3,
  kMarker,
  0,
  NArcInfoFlags::kFindSignature,
  NULL)

}}
