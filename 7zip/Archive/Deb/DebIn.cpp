// Archive/DebIn.cpp

#include "StdAfx.h"

#include "DebIn.h"
#include "DebHeader.h"

#include "Windows/Defs.h"

namespace NArchive {
namespace NDeb {

using namespace NHeader;

HRESULT CInArchive::ReadBytes(void *data, UINT32 size, UINT32 &processedSize)
{
  RINOK(m_Stream->Read(data, size, &processedSize));
  m_Position += processedSize;
  return S_OK;
}

HRESULT CInArchive::Open(IInStream *inStream)
{
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_Position));
  char signature[kSignatureLen];
  UINT32 processedSize;
  RINOK(inStream->Read(signature, kSignatureLen, &processedSize));
  m_Position += processedSize;
  if (processedSize != kSignatureLen)
    return S_FALSE;
  if (memcmp(signature, kSignature, kSignatureLen) != 0)
    return S_FALSE;
  m_Stream = inStream;
  return S_OK;
}

static bool CheckString(const char *srcString, int numChars, int radix)
{
  for(int i = 0; i < numChars; i++)
  {
    char c = srcString[i];
    if (c == 0)
      return true;
    if (c >= '0' && c <= '0' + radix - 1)
      continue;
    if (c != ' ')
      return false;
  }
  return true;
}
static bool CheckOctalString(const char *srcString, int numChars)
  { return CheckString(srcString, numChars, 8); }
static bool CheckDecimalString(const char *srcString, int numChars)
  { return CheckString(srcString, numChars, 10); }

#define ReturnIfBadOctal(x, y) { if (!CheckOctalString((x), (y))) return S_FALSE; }
#define ReturnIfBadDecimal(x, y) { if (!CheckDecimalString((x), (y))) return S_FALSE; }

static UINT32 StringToNumber(const char *srcString, int numChars, int radix)
{
  AString modString;
  for (int i = 0; i < numChars; i++)
    modString += srcString[i];
  char *endPtr;
  return strtoul(modString, &endPtr, radix);
}
static UINT32 OctalToNumber(const char *srcString, int numChars)
  { return StringToNumber(srcString, numChars, 8); }
static UINT32 DecimalToNumber(const char *srcString, int numChars)
  { return StringToNumber(srcString, numChars, 10); }

HRESULT CInArchive::GetNextItemReal(bool &filled, CItemEx &item)
{
  filled = false;

  CHeader header;
  UINT32 processedSize;
  item.HeaderPosition = m_Position;
  RINOK(ReadBytes(&header, sizeof(header), processedSize));
  if (processedSize < sizeof(header))
    return S_OK;
  
  char tempString[kNameSize + 1];
  strncpy(tempString, header.Name, kNameSize);
  tempString[kNameSize] = '\0';
  item.Name = tempString;
  item.Name.Trim();

  for (int i = 0; i < item.Name.Length(); i++)
    if (((BYTE)item.Name[i]) < 0x20)
      return S_FALSE;

  ReturnIfBadDecimal(header.ModificationTime, kTimeSize);
  ReturnIfBadOctal(header.Mode, kModeSize);
  ReturnIfBadDecimal(header.Size, kSizeSize);

  item.ModificationTime = DecimalToNumber(header.ModificationTime, kTimeSize);
  item.Mode = OctalToNumber(header.Mode, kModeSize);
  item.Size = DecimalToNumber(header.Size, kSizeSize);

  filled = true;
  return S_OK;
}

HRESULT CInArchive::GetNextItem(bool &filled, CItemEx &item)
{
  while(true)
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
  return S_OK;
}

HRESULT CInArchive::Skeep(UINT64 numBytes)
{
  UINT64 newPostion;
  RINOK(m_Stream->Seek(numBytes, STREAM_SEEK_CUR, &newPostion));
  m_Position += numBytes;
  if (m_Position != newPostion)
    return E_FAIL;
  return S_OK;
}

HRESULT CInArchive::SkeepData(UINT64 dataSize)
{
  return Skeep((dataSize + 1) & 0xFFFFFFFFFFFFFFFE);
}

}}
