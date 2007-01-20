// Archive/DebIn.cpp

#include "StdAfx.h"

#include "DebIn.h"
#include "DebHeader.h"

#include "Common/StringToInt.h"
#include "Windows/Defs.h"

#include "../../Common/StreamUtils.h"

namespace NArchive {
namespace NDeb {

using namespace NHeader;

HRESULT CInArchive::ReadBytes(void *data, UInt32 size, UInt32 &processedSize)
{
  RINOK(ReadStream(m_Stream, data, size, &processedSize));
  m_Position += processedSize;
  return S_OK;
}

HRESULT CInArchive::Open(IInStream *inStream)
{
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_Position));
  char signature[kSignatureLen];
  UInt32 processedSize;
  RINOK(ReadStream(inStream, signature, kSignatureLen, &processedSize));
  m_Position += processedSize;
  if (processedSize != kSignatureLen)
    return S_FALSE;
  if (memcmp(signature, kSignature, kSignatureLen) != 0)
    return S_FALSE;
  m_Stream = inStream;
  return S_OK;
}

static void MyStrNCpy(char *dest, const char *src, int size)
{
  for (int i = 0; i < size; i++)
  {
    char c = src[i];
    dest[i] = c;
    if (c == 0)
      break;
  }
}

static bool OctalToNumber(const char *s, int size, UInt64 &res)
{
  char sz[32];
  MyStrNCpy(sz, s, size);
  sz[size] = 0;
  const char *end;
  int i;
  for (i = 0; sz[i] == ' '; i++);
  res = ConvertOctStringToUInt64(sz + i, &end);
  return (*end == ' ' || *end == 0);
}

static bool OctalToNumber32(const char *s, int size, UInt32 &res)
{
  UInt64 res64;
  if (!OctalToNumber(s, size, res64))
    return false;
  res = (UInt32)res64;
  return (res64 <= 0xFFFFFFFF);
}

static bool DecimalToNumber(const char *s, int size, UInt64 &res)
{
  char sz[32];
  MyStrNCpy(sz, s, size);
  sz[size] = 0;
  const char *end;
  int i;
  for (i = 0; sz[i] == ' '; i++);
  res = ConvertStringToUInt64(sz + i, &end);
  return (*end == ' ' || *end == 0);
}

static bool DecimalToNumber32(const char *s, int size, UInt32 &res)
{
  UInt64 res64;
  if (!DecimalToNumber(s, size, res64))
    return false;
  res = (UInt32)res64;
  return (res64 <= 0xFFFFFFFF);
}

#define RIF(x) { if (!(x)) return S_FALSE; }


HRESULT CInArchive::GetNextItemReal(bool &filled, CItemEx &item)
{
  filled = false;

  char header[NHeader::kHeaderSize];
  const char *cur = header;

  UInt32 processedSize;
  item.HeaderPosition = m_Position;
  RINOK(ReadBytes(header, sizeof(header), processedSize));
  if (processedSize < sizeof(header))
    return S_OK;
  
  char tempString[kNameSize + 1];
  MyStrNCpy(tempString, cur, kNameSize);
  cur += kNameSize;
  tempString[kNameSize] = '\0';
  item.Name = tempString;
  item.Name.Trim();

  for (int i = 0; i < item.Name.Length(); i++)
    if (((Byte)item.Name[i]) < 0x20)
      return S_FALSE;

  RIF(DecimalToNumber32(cur, kTimeSize, item.ModificationTime));
  cur += kTimeSize;

  cur += 6 + 6;
  
  RIF(OctalToNumber32(cur, kModeSize, item.Mode));
  cur += kModeSize;

  RIF(DecimalToNumber(cur, kSizeSize, item.Size));
  cur += kSizeSize;

  filled = true;
  return S_OK;
}

HRESULT CInArchive::GetNextItem(bool &filled, CItemEx &item)
{
  for (;;)
  {
    RINOK(GetNextItemReal(filled, item));
    if (!filled)
      return S_OK;
    if (item.Name.CompareNoCase("debian-binary") != 0)
      return S_OK;
    if (item.Size != 4)
      return S_OK;
    SkeepData(item.Size);
  }
}

HRESULT CInArchive::Skeep(UInt64 numBytes)
{
  UInt64 newPostion;
  RINOK(m_Stream->Seek(numBytes, STREAM_SEEK_CUR, &newPostion));
  m_Position += numBytes;
  if (m_Position != newPostion)
    return E_FAIL;
  return S_OK;
}

HRESULT CInArchive::SkeepData(UInt64 dataSize)
{
  return Skeep((dataSize + 1) & (~((UInt64)0x1)));
}

}}
