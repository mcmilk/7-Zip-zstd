// Archive/LzhIn.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/Buffer.h"

#include "../../Common/StreamUtils.h"

#include "LzhIn.h"

namespace NArchive {
namespace NLzh {
 
HRESULT CInArchive::ReadBytes(void *data, UInt32 size, UInt32 &processedSize)
{
  RINOK(ReadStream(m_Stream, data, size, &processedSize));
  m_Position += processedSize;
  return S_OK;
}

HRESULT CInArchive::CheckReadBytes(void *data, UInt32 size)
{
  UInt32 processedSize;
  RINOK(ReadBytes(data, size, processedSize));
  return (processedSize == size) ? S_OK: S_FALSE;
}

HRESULT CInArchive::Open(IInStream *inStream)
{
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_Position));
  m_Stream = inStream;
  return S_OK;
}

static const Byte *ReadUInt32(const Byte *p, UInt32 &v)
{
  v = 0;
  for (int i = 0; i < 4; i++)
    v |= ((UInt32)(*p++) << (i * 8));
  return p;
}

static const Byte *ReadUInt16(const Byte *p, UInt16 &v)
{
  v = 0;
  for (int i = 0; i < 2; i++)
    v |= ((UInt16)(*p++) << (i * 8));
  return p;
}

static const Byte *ReadString(const Byte *p, size_t size, AString &s)
{
  s.Empty();
  for (size_t i = 0; i < size; i++)
  {
    char c = p[i];
    if (c == 0)
      break;
    s += c;
  }
  return p + size;
}

static Byte CalcSum(const Byte *data, size_t size)
{
  Byte sum = 0;
  for (size_t i = 0; i < size; i++)
    sum = (Byte)(sum + data[i]);
  return sum;
}

HRESULT CInArchive::GetNextItem(bool &filled, CItemEx &item)
{
  filled = false;

  UInt32 processedSize;
  Byte startHeader[2];
  RINOK(ReadBytes(startHeader, 2, processedSize))
  if (processedSize == 0)
    return S_OK;
  if (processedSize == 1)
    return (startHeader[0] == 0) ? S_OK: S_FALSE;
  if (startHeader[0] == 0 && startHeader[1] == 0)
    return S_OK;

  Byte header[256];
  const UInt32 kBasicPartSize = 22;
  RINOK(ReadBytes(header, kBasicPartSize, processedSize));
  if (processedSize != kBasicPartSize)
    return (startHeader[0] == 0) ? S_OK: S_FALSE;

  const Byte *p = header;
  memmove(item.Method, p, kMethodIdSize);
  if (!item.IsValidMethod())
    return S_OK;
  p += kMethodIdSize;
  p = ReadUInt32(p, item.PackSize);
  p = ReadUInt32(p, item.Size);
  p = ReadUInt32(p, item.ModifiedTime);
  item.Attributes = *p++;
  item.Level = *p++;
  if (item.Level > 2)
    return S_FALSE;
  UInt32 headerSize;
  if (item.Level < 2)
  {
    headerSize = startHeader[0];
    if (headerSize < kBasicPartSize)
      return S_FALSE;
    UInt32 remain = headerSize - kBasicPartSize;
    RINOK(CheckReadBytes(header + kBasicPartSize, remain));
    if (startHeader[1] != CalcSum(header, headerSize))
      return S_FALSE;
    size_t nameLength = *p++;
    if ((p - header) + nameLength + 2 > headerSize)
      return S_FALSE;
    p = ReadString(p, nameLength, item.Name);
  }
  else
   headerSize = startHeader[0] | ((UInt32)startHeader[1] << 8);
  p = ReadUInt16(p, item.CRC);
  if (item.Level != 0)
  {
    if (item.Level == 2)
    {
      RINOK(CheckReadBytes(header + kBasicPartSize, 2));
    }
    if ((size_t)(p - header) + 3 > headerSize)
      return S_FALSE;
    item.OsId = *p++;
    UInt16 nextSize;
    p = ReadUInt16(p, nextSize);
    while (nextSize != 0)
    {
      if (nextSize < 3)
        return S_FALSE;
      if (item.Level == 1)
      {
        if (item.PackSize < nextSize)
          return S_FALSE;
        item.PackSize -= nextSize;
      }
      CExtension ext;
      RINOK(CheckReadBytes(&ext.Type, 1))
      nextSize -= 3;
      ext.Data.SetCapacity(nextSize);
      RINOK(CheckReadBytes((Byte *)ext.Data, nextSize))
      item.Extensions.Add(ext);
      Byte hdr2[2];
      RINOK(CheckReadBytes(hdr2, 2));
      ReadUInt16(hdr2, nextSize);
    }
  }
  item.DataPosition = m_Position;
  filled = true;
  return S_OK;
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

}}
